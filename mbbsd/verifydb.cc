extern "C" {
#include "bbs.h"
#include "daemons.h"
}

#ifdef USE_VERIFYDB

#include <optional>
#include <string>

#include "flatbuffers/flatbuffers.h"
#include "verifydb.h"
#include "verifydb.fbs.h"

namespace {

bool verifydb_transact(const void *msg, size_t len, Bytes *out) {
  int fd = toconnect(REGMAILD_ADDR);
  if (fd < 0)
    return false;

  // Send request.
  verifydb_message_t rep = {};
  rep.header.cb = sizeof(rep.header) + len;
  rep.header.operation = VERIFYDB_MESSAGE;
  if (towrite(fd, &rep.header, sizeof(rep.header)) != sizeof(rep.header) ||
      towrite(fd, msg, len) != len)
    return false;

  // Read reply.
  if (toread(fd, &rep.header, sizeof(rep.header)) != sizeof(rep.header))
    return false;

  out->resize(rep.header.cb - sizeof(rep.header));
  if (toread(fd, &out->front(), out->size()) != out->size())
    return false;

  return true;
}

}  // namespace

std::optional<std::string> FormatVerify(int32_t vmethod, const char *vkey,
                                        bool ansi_color) {
  std::string s;
  switch (vmethod) {
  case VMETHOD_EMAIL:
    s = "(�q�l�H�c) ";
    if (vkey) {
      if (ansi_color)
        s += ANSI_COLOR(1);
      s += vkey;
      if (ansi_color)
        s += ANSI_RESET;
    } else
      s += "?";
    return s;
  }
  return std::nullopt;
}

std::optional<std::string> FormatVerify(int32_t vmethod,
                                        const flatbuffers::String *vkey,
                                        bool ansi_color) {
  return FormatVerify(vmethod, vkey ? vkey->c_str() : nullptr, ansi_color);
}

bool is_entry_user_present(const VerifyDb::Entry *entry) {
  const char *userid = "";
  if (entry->userid())
    userid = entry->userid()->c_str();
  if (!is_validuserid(userid))
    return false;

  userec_t u;
  if (passwd_load_user(userid, &u) < 0)
    return false;

  return entry->generation() == u.firstlogin;
}

bool verifydb_getuser(const char *userid, int64_t generation, Bytes *buf,
                      const VerifyDb::GetReply **reply) {
  // Create request.
  flatbuffers::FlatBufferBuilder fbb;
  auto entry = VerifyDb::CreateEntryDirect(fbb, userid, generation);
  auto match_fields = fbb.CreateVector<int8_t>(
      {VerifyDb::Field_UserId, VerifyDb::Field_Generation});
  auto req = VerifyDb::CreateMessage(
      fbb, VerifyDb::AnyMessage_GetRequest,
      VerifyDb::CreateGetRequest(fbb, entry, match_fields).Union());
  fbb.Finish(req);

  // Transact.
  if (!verifydb_transact(fbb.GetBufferPointer(), fbb.GetSize(), buf))
    return false;

  // Verify reply.
  auto verifier = flatbuffers::Verifier(&buf->front(), buf->size());
  if (!VerifyDb::VerifyMessageBuffer(verifier))
    return false;

  // Expect reply is a GetReply message.
  *reply = VerifyDb::GetMessage(&buf->front())->message_as_GetReply();
  return *reply != nullptr;
}

bool verifydb_getverify(int32_t vmethod, const char *vkey, Bytes *buf,
                        const VerifyDb::GetReply **reply) {
  // Create request.
  flatbuffers::FlatBufferBuilder fbb;
  auto entry = VerifyDb::CreateEntryDirect(fbb, nullptr, 0, vmethod, vkey);
  auto match_fields = fbb.CreateVector<int8_t>({VerifyDb::Field_Verify});
  auto req = VerifyDb::CreateMessage(
      fbb, VerifyDb::AnyMessage_GetRequest,
      VerifyDb::CreateGetRequest(fbb, entry, match_fields).Union());
  fbb.Finish(req);

  // Transact.
  if (!verifydb_transact(fbb.GetBufferPointer(), fbb.GetSize(), buf))
    return false;

  // Verify reply.
  auto verifier = flatbuffers::Verifier(&buf->front(), buf->size());
  if (!VerifyDb::VerifyMessageBuffer(verifier))
    return false;

  // Expect reply is a GetReply message.
  *reply = VerifyDb::GetMessage(&buf->front())->message_as_GetReply();
  return *reply != nullptr;
}

int verifydb_count_by_verify(int32_t vmethod, const char *vkey, int *count_self,
                             int *count_other) {
  const VerifyDb::GetReply *reply;
  Bytes buf;
  if (!verifydb_getverify(vmethod, vkey, &buf, &reply) || !reply->ok() ||
      !reply->entry())
    return -1;

  *count_self = 0;
  *count_other = 0;
  for (const auto *entry : *reply->entry()) {
    if (!is_entry_user_present(entry))
      continue;

    if (strcasecmp(cuser.userid, entry->userid()->c_str()) == 0)
      (*count_self)++;
    else
      (*count_other)++;
  }
  return 0;
}

int verifydb_set(const char *userid, int64_t generation, int32_t vmethod,
                 const char *vkey, int64_t timestamp) {
  // Create request.
  flatbuffers::FlatBufferBuilder fbb;
  auto entry = VerifyDb::CreateEntryDirect(fbb, userid, generation, vmethod,
                                           vkey, timestamp);
  auto req = VerifyDb::CreateMessage(
      fbb, VerifyDb::AnyMessage_PutRequest,
      VerifyDb::CreatePutRequest(fbb, entry, VerifyDb::Conflict_Replace)
          .Union());
  fbb.Finish(req);

  // Transact.
  Bytes out;
  if (!verifydb_transact(fbb.GetBufferPointer(), fbb.GetSize(), &out))
    return -1;

  // Verify reply.
  auto verifier = flatbuffers::Verifier(&out.front(), out.size());
  if (!VerifyDb::VerifyMessageBuffer(verifier))
    return -1;

  // Expect reply is a PutReply message.
  const auto *reply = VerifyDb::GetMessage(&out.front())->message_as_PutReply();
  if (!reply || !reply->ok())
    return -1;
  return 0;
}

#endif
