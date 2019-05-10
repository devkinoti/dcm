#include "dcm/data_element.h"

#include <cctype>
#include <iostream>
#include <sstream>

#include "dcm/data_dict.h"
#include "dcm/util.h"
#include "dcm/visitor.h"

namespace dcm {

// -----------------------------------------------------------------------------

namespace {

// The behavior of `std::isalpha` and other functions from <cctype> is undefined
// if the argument's value is neither representable as `unsigned char` nor equal
// to `EOF`. To use these functions safely with plain `chars` (or `signed chars`
// ), the argument should first be converted to `unsigned char`.

// See: https://en.cppreference.com/w/cpp/string/byte/isalpha

inline bool IsAlpha(char ch) {
  return std::isalpha(static_cast<unsigned char>(ch)) != 0;
}

inline bool IsUpper(char ch) {
  return std::isupper(static_cast<unsigned char>(ch)) != 0;
}

inline bool IsDigit(char ch) {
  return std::isdigit(static_cast<unsigned char>(ch)) != 0;
}

// -----------------------------------------------------------------------------

// Application Entity
bool CheckAE(const std::string& value) {
  // Max: 16

  if (value.empty() || value.size() > 16) {
    return false;
  }

  return true;
}

// Age String
bool CheckAS(const std::string& value) {
  // Fix: 4
  // Format: nnnD, nnnW, nnnM, nnnY.
  // E.g., "018M".

  if (value.size() != 4) {
    return false;
  }

  for (std::size_t i = 0; i < 3; ++i) {
    if (!IsDigit(value[i])) {
      return false;
    }
  }

  if (value[3] != 'D' || value[3] != 'W' || value[3] != 'M' ||
      value[3] != 'Y') {
    return false;
  }

  return true;
}

// Code String
bool CheckCS(const std::string& value) {
  // Max: 16
  // Uppercase characters, 0-9, SPACE, underscore (_).
  // E.g., "CD123_4"

  if (value.size() > 16) {
    return false;
  }

  auto is_valid = [](char c) {
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' ||
      c == '_';
  };

  for (char c : value) {
    if (!is_valid(c)) {
      return false;
    }
  }
  return true;
}

// Date
bool CheckDA(const std::string& value) {
  // Fix: 8
  // YYYYMMDD; 0-9.
  // Example: "20050822"

  if (value.size() != 8) {
    return false;
  }

  for (char c : value) {
    if (!IsDigit(c)) {
      return false;
    }
  }

  // TODO: Check the range.

  return true;
}

// Time
bool CheckTM(const std::string& value) {
  // Max: 16
  // A string of characters of the format HHMMSS.FRAC;
  // Example: "183200.00"
  // Allowed characters: 0-9, and period(.)

  if (value.size() > 16) {
    return false;
  }

  // TODO

  return true;
}

// Date Time
bool CheckDT(const std::string& value) {
  // Max: 26
  // Concatenated datetime string in the format: YYYYMMDDHHMMSS.FFFFFF&ZZXX
  // Example: "20050812183000.00"
  // Allowed characters: 0-9, plus(+), minus(-) and period(.)

  if (value.size() > 26) {
    return false;
  }

  // TODO

  return true;
}

// Decimal String
bool CheckDS(const std::string& value) {
  // Max: 16
  // A string of characters representing either a fixed point number or a
  // floating point number.
  // Example: "12345.67", "-5.0e3"
  // Allowed characters: 0-9, plus(+), minus(-), E, e, and period(.)

  if (value.size() > 16) {
    return false;
  }

  // TODO: Check more.

  return true;
}

// Integer String
bool CheckIS(const std::string& value) {
  // Max: 12
  // A string of characters representing an Integer in base-10 (decimal), shall
  // contain only the characters 0 - 9, with an optional leading "+"or "-".

  if (value.size() > 12) {
    return false;
  }

  std::size_t i = 0;

  if (value.size() > 1) {
    if (value[0] == '+' || value[0] == '-') {
      ++i;
    }
  }

  for (; i < value.size(); ++i) {
    if (!IsDigit(value[i])) {
      return false;
    }
  }

  return true;
}

// Long String
bool CheckLO(const std::string& value) {
  // Max: 64

  if (value.size() > 64) {
    return false;
  }

  return true;
}

// Short Text
bool CheckST(const std::string& value) {
  // Max: 1024
  // A character string that may contain one or more paragraphs.

  if (value.size() > 1024) {
    return false;
  }

  return true;
}

// Long Text
bool CheckLT(const std::string& value) {
  // Max: 10240

  if (value.size() > 10240) {
    return false;
  }

  return true;
}

// Unlimited Text
bool CheckUT(const std::string& value) {
  // Max: 4,294,967,294 (2^32 - 2)

  return true;
}

// Person Name
bool CheckPN(const std::string& value) {
  // Max: 64

  if (value.size() > 64) {
    return false;
  }

  // TODO

  return true;
}

// Short String
bool CheckSH(const std::string& value) {
  // Max: 16
  // Example: telephone numbers, IDs

  if (value.size() > 16) {
    return false;
  }

  return true;
}

// Unlimited Characters
// TODO
bool CheckUC(const std::string& value) {
  return true;
}

// Unique Identifier(UID)
bool CheckUI(const std::string& value) {
  // Max: 64
  // A character string containing a UID that is used to uniquely identify a
  // wide variety of items.
  // Example: "1.2.840.10008.1.1"

  if (value.size() > 64) {
    return false;
  }

  // TODO

  return true;
}

// Universal Resource Identifier
bool CheckUR(const std::string& value) {
  // Max: 64
  // A string of characters that identifies a URI or a URL as defined in
  // [RFC3986]. Leading spaces are not allowed. Trailing spaces shallbe ignored.
  // Data Elements with this VR shall not be multi-valued.
  // Example: "1.2.840.10008.1.1"

  // TODO

  return true;
}

// -----------------------------------------------------------------------------

bool CheckStringValue(VR vr, const std::string& value) {
  switch (vr.code()) {
    case VR::AE:
      return CheckAE(value);

    case VR::AS:
      return CheckAS(value);

    case VR::CS:
      return CheckCS(value);

    case VR::DA:
      return CheckDA(value);

    case VR::TM:
      return CheckTM(value);

    case VR::DT:
      return CheckDT(value);

    case VR::DS:
      return CheckDS(value);

    case VR::IS:
      return CheckIS(value);

    case VR::LO:
      return CheckLO(value);

    case VR::ST:
      return CheckST(value);

    case VR::LT:
      return CheckLT(value);

    case VR::UT:
      return CheckUT(value);

    case VR::PN:
      return CheckPN(value);

    case VR::SH:
      return CheckSH(value);

    case VR::UC:
      return CheckUC(value);

    case VR::UI:
      return CheckUI(value);

    case VR::UR:
      return CheckUR(value);

    default:
      break;
  }

  return false;
}

}  // namespace

// -----------------------------------------------------------------------------

DataElement::DataElement(Tag tag, VR vr, ByteOrder byte_order)
    : tag_(tag), vr_(vr), byte_order_(byte_order), length_(0) {
}

DataElement::DataElement(Tag tag, ByteOrder byte_order)
    : tag_(tag), byte_order_(byte_order), length_(0) {
  vr_ = DataDict::GetVR(tag);
}

void DataElement::Accept(Visitor& visitor) const {
  visitor.VisitDataElement(this);
}

bool DataElement::SetBuffer(Buffer&& buffer) {
  if (buffer.size() % 2 != 0) {
    return false;
  }

  // TODO: Redundent (only useful for SQ and SQ item prefix tags).
  length_ = buffer.size();

  buffer_ = std::move(buffer);

  return true;
}

bool DataElement::GetString(std::string* value) const {
  if (buffer_.empty()) {
    value->clear();
    return true;
  }

  std::size_t size = buffer_.size();

  if (buffer_.back() == ' ') {
    // Remove padding space.
    --size;
  }

  if (size > 0 && buffer_[size - 1] == '\0') {
    // Some strings end with a redundent '\0', remove it.
    --size;
  }

  value->assign(&buffer_[0], size);

  return true;
}

bool DataElement::SetString(const std::string& value) {
  if (!CheckStringValue(vr_, value)) {
    return false;
  }

  bool padding = (value.size() % 2 == 1);

  length_ = value.size();
  if (padding) {
    length_ += 1;
  }

  buffer_.resize(length_);

  if (length_ > 0) {
    std::memcpy(&buffer_[0], value.data(), value.size());
  }

  if (padding) {
    buffer_[length_ - 1] = ' ';
  }

  return true;
}

void DataElement::Print(std::ostream& os) const {
  tag_.Print(os);

  const char* TAB = "\t";

  os << TAB;

  if (vr_.IsUnknown()) {
    os << "--";
  } else {
    os << vr_.byte1() << vr_.byte2();
  }

  os << TAB;

  if (length_ != kUndefinedLength) {
    os << length_;
  } else {
    os << "-1";
  }

  os << TAB;

  PrintValue(os);
}

void DataElement::PrintValue(std::ostream& os) const {
  switch (vr_.code()) {
    case VR::AT:  // Attribute Tag
      // Ordered pair of 16-bit (2-byte) unsigned integers that is the value
      // of a Data Element Tag.
      // TODO
      break;

    case VR::OB:
    case VR::OW:
      os << "<Binary>";
      break;

    case VR::AE:  // Application Entity
    case VR::AS:  // Age String
    case VR::CS:  // Code String
    case VR::DS:  // Decimal String
    case VR::SH:  // Short String
    case VR::LO:  // Long String
    case VR::ST:  // Short Text
    case VR::LT:  // Long Text
    case VR::IS:  // Integer String
    case VR::UI:  // UID
    case VR::UR:  // URI/URL
    case VR::DA:  // Date
    case VR::TM:  // Time
    case VR::PN:  // Person Name
    {
      std::string str;
      GetString(&str);
      os << str;
    }
    break;

    case VR::US:  // Unsigned Short
    {
      std::uint16_t value = 0;
      GetUint16(&value);
      os << value;
    }
    break;

    case VR::SS:  // Signed Short
    {
      std::int16_t value = 0;
      GetInt16(&value);
      os << value;
    }
    break;

    case VR::UL:  // Unsigned Long
    {
      std::uint32_t value = 0;
      GetUint32(&value);
      os << value;
    }
    break;

    case VR::SL:  // Signed Long
    {
      std::int32_t value = 0;
      GetInt32(&value);
      os << value;
    }
    break;

    case VR::FL:  // Floating Point Single
    {
      float32_t value = 0.0;
      GetFloat32(&value);
      os << value;
    }
    break;

    case VR::FD:  // Floating Point Double
    {
      float64_t value = 0.0;
      GetFloat64(&value);
      os << value;
    }
    break;

    default:
      break;
  }
}

std::string DataElement::PrintValue() const {
  std::stringstream ss;
  PrintValue(ss);
  return ss.str();
}

bool DataElement::GetNumber(VR vr, size_t size, void* value) const {
  if (vr_ != vr) {
    return false;
  }

  GetBytes(value, size);

  AdjustBytes(value, size);

  return true;
}

bool DataElement::SetNumber(VR vr, size_t size, void* value) {
  if (vr_ != vr) {
    return false;
  }

  AdjustBytes(value, size);

  SetBytes(value, size);

  return true;
}

void DataElement::GetBytes(void* value, std::size_t length) const {
  assert(length == length_);  // TODO

  memcpy(value, &buffer_[0], length_);
}

void DataElement::SetBytes(void* value, std::size_t length) {
  assert(length != 0);  // TODO: length % 2 == 0

  length_ = length;

  if (buffer_.size() != length_) {
    buffer_.resize(length_);
  }

  memcpy(&buffer_[0], value, length_);
}

void DataElement::AdjustBytes(void* value, std::size_t size) const {
  if (byte_order_ != kByteOrderOS) {
    if (size == 2) {
      Swap16(value);
    } else if (size == 4) {
      Swap32(value);
    } else if (size == 8) {
      Swap64(value);
    }
  }
}

}  // namespace dcm
