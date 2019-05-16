#include "dcm/defs.h"

#include <array>
#include <iomanip>
#include <map>

#include "dcm/singleton_base.h"
#include "dcm/util.h"

namespace dcm {

// -----------------------------------------------------------------------------

static union {
  char c[4];
  unsigned long mylong;
} byte_order_test = { { 'l', '?', '?', 'b' } };

const ByteOrder kByteOrderOS =
    ((char)byte_order_test.mylong == 'l') ? ByteOrder::LE : ByteOrder::BE;

// -----------------------------------------------------------------------------

class VRInfo {
public:
  int code;  // VR::Code

  // The length of word; used for byte reversing in little/big endian
  // conversion. For numeric VRs only ((FL, FD, OL, OW, etc.).
  std::uint32_t word_size;

  // The maximum length of the data.
  std::uint32_t max_data_length;
};

// VR dictionary.
// A singleton to cache all valid VRs.
class VRDict : public SingletonBase<VRDict> {
public:
  ~VRDict() = default;

  bool IsValid(int code) const {
    return vr_map_.find(code) != vr_map_.end();
  }

private:
  friend class SingletonBase<VRDict>;

  VRDict() {
    RegisterVR(VR::AE, 0, 16);
    RegisterVR(VR::AS, 0, 0);
    RegisterVR(VR::AT, 2, 0);
    RegisterVR(VR::CS, 0, 16);
    RegisterVR(VR::DA, 0, 0);
    RegisterVR(VR::DS, 0, 16);
    RegisterVR(VR::DT, 0, 26);
    RegisterVR(VR::FL, 4, 0);
    RegisterVR(VR::FD, 8, 0);
    RegisterVR(VR::IS, 0, 12);
    RegisterVR(VR::LO, 0, 64);
    RegisterVR(VR::LT, 0, 10240);
    RegisterVR(VR::OB, 0, 0);
    RegisterVR(VR::OF, 4, 0);
    RegisterVR(VR::OL, 4, 0);
    RegisterVR(VR::OW, 2, 0);
    RegisterVR(VR::PN, 0, 64);
    RegisterVR(VR::SH, 0, 16);
    RegisterVR(VR::SL, 4, 0);
    RegisterVR(VR::SQ, 0, 0);
    RegisterVR(VR::SS, 2, 0);
    RegisterVR(VR::ST, 0, 1024);
    RegisterVR(VR::TM, 0, 16);
    RegisterVR(VR::UI, 0, 64);
    RegisterVR(VR::UL, 4, 0);
    RegisterVR(VR::UN, 0, 0);
    RegisterVR(VR::US, 2, 0);
    RegisterVR(VR::UT, 0, 0);
  }

  void RegisterVR(int code, std::uint32_t word_size,
                  std::uint32_t max_data_length) {
    vr_map_[code] = { code, word_size, max_data_length };
  }

private:
  std::map<int, VRInfo> vr_map_;
};

VR::VR(const char bytes[2]) {
  int code = MAKE_VR_CODE(bytes[0], bytes[1]);

  if (VRDict::Instance()->IsValid(code)) {
    code_ = static_cast<VR::Code>(code);
  } else {
    code_ = UN;
  }
}

bool VR::SetBytes(const char bytes[2]) {
  int code = MAKE_VR_CODE(bytes[0], bytes[1]);

  if (!VRDict::Instance()->IsValid(code)) {
    return false;
  }

  code_ = static_cast<VR::Code>(code);
  return true;
}

bool VR::Is16BitsFollowingReversed() const {
  if (code_ == VR::OB || code_ == VR::OD || code_ == VR::OF ||
      code_ == VR::OL || code_ == VR::OW || code_ == VR::SQ ||
      code_ == VR::UN || code_ == VR::UC || code_ == VR::UR ||
      code_ == VR::UT) {
    return true;
  }
  return false;
}

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

bool VR::IsString() const {
  static const Code kVRCodes[] = {
    AE, AS, CS, DA, TM, DT, DS, IS, LO, ST, LT, UT, PN, SH, UC, UI, UR,
  };

  const auto end = kVRCodes + ARRAY_SIZE(kVRCodes);

  return std::find(kVRCodes, end, code_) != end;
}

bool VR::IsBackSlashVM() const {
  static const Code kVRCodes[] = {
    AE, AS, CS, DA, DS, DT, TM, IS, UI,
    SH, LO, PN, UC,
  };

  const auto end = kVRCodes + ARRAY_SIZE(kVRCodes);

  return std::find(kVRCodes, end, code_) != end;
}

// -----------------------------------------------------------------------------

Tag Tag::SwapBytes() const {
  return Tag(SwapUint16(group_), SwapUint16(element_));
}

void Tag::Print(std::ostream& os, bool uppercase, bool as_uint32,
                const char* separator) const {
  std::ios::fmtflags old_flags = os.flags(std::ios::right | std::ios::hex);

  if (uppercase) {
    os.setf(std::ios::uppercase);
  }

  os << std::setfill('0');

  if (as_uint32) {
    os << std::setw(8) << ToUint32();
  } else {
    os << std::setw(4) << group_;
    os << separator;
    os << std::setw(4) << element_;
  }

  os.flags(old_flags);
}

}  // namespace dcm
