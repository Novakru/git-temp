/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2023/06/28.
//

#include <sstream>
#include "sql/parser/value.h"
#include "storage/field/field.h"
#include "common/log/log.h"
#include "common/lang/comparator.h"
#include "common/lang/string.h"
#include <ctime>
#include <ctype.h>
#include <cstring>

const char *ATTR_TYPE_NAME[] = {"undefined", "chars", "ints", "dates", "floats", "booleans"};
const char *ATTR_TYPE_NAME_UPPERCASE[] = {"UNDEFINED", "CHARS", "INTS", "DATES", "FLOATS", "BOOLEANS"};

const char *attr_type_to_string(AttrType type)
{
  if (type >= UNDEFINED && type <= FLOATS) {
    return ATTR_TYPE_NAME[type];
  }
  return "unknown";
}
AttrType attr_type_from_string(const char *s)
{
  for (unsigned int i = 0; i < sizeof(ATTR_TYPE_NAME) / sizeof(ATTR_TYPE_NAME[0]); i++) {
    if (0 == strcmp(ATTR_TYPE_NAME[i], s)) {
      return (AttrType)i;
    }
  }
  return UNDEFINED;
}

Value::Value(int val)
{
  set_int(val);
}

Value::Value(float val)
{
  set_float(val);
}

Value::Value(bool val)
{
  set_boolean(val);
}

Value::Value(const char *s, int len /*= 0*/)
{
  set_string(s, len);
}

Value::Value(const char *s, int len, int flag)
{ 
  int val = chars_to_timestamp(s); // 存储TimeStamp
  set_date(val);
}

void Value::set_data(char *data, int length)
{
  switch (attr_type_) {
    case CHARS: {
      set_string(data, length);
    } break;
    case INTS: {
      num_value_.int_value_ = *(int *)data;
      length_ = length;
    } break;
    case DATES: {
      num_value_.date_value_ = *(int *)data;
      length_ = length;
    } break;
    case FLOATS: {
      num_value_.float_value_ = *(float *)data;
      length_ = length;
    } break;
    case BOOLEANS: {
      num_value_.bool_value_ = *(int *)data != 0;
      length_ = length;
    } break;
    default: {
      LOG_WARN("unknown data type: %d", attr_type_);
    } break;
  }
}
void Value::set_int(int val)
{
  attr_type_ = INTS;
  num_value_.int_value_ = val;
  length_ = sizeof(val);
}
void Value::set_date(int val)
{
  attr_type_ = DATES;
  num_value_.date_value_= val;
  length_ = sizeof(val);
}
void Value::set_float(float val)
{
  attr_type_ = FLOATS;
  num_value_.float_value_ = val;
  length_ = sizeof(val);
}
void Value::set_boolean(bool val)
{
  attr_type_ = BOOLEANS;
  num_value_.bool_value_ = val;
  length_ = sizeof(val);
}
void Value::set_string(const char *s, int len /*= 0*/)
{
  attr_type_ = CHARS;
  if (len > 0) {
    len = strnlen(s, len);
    str_value_.assign(s, len);
  } else {
    str_value_.assign(s);
  }
  length_ = str_value_.length();
}
void Value::set_value(const Value &value)
{
  switch (value.attr_type_) {
    case INTS: {
      set_int(value.get_int());
    } break;
    case DATES: {
      set_date(value.get_date());
    } break;
    case FLOATS: {
      set_float(value.get_float());
    } break;
    case CHARS: {
      set_string(value.get_string().c_str());
    } break;
    case BOOLEANS: {
      set_boolean(value.get_boolean());
    } break;
    case UNDEFINED: {
      ASSERT(false, "got an invalid value type");
    } break;
  }
}

const char *Value::data() const
{
  switch (attr_type_) {
    case CHARS: {
      return str_value_.c_str();
    } break;
    default: {
      return (const char *)&num_value_;
    } break;
  }
}

std::string Value::to_string() const // 不同类型的数据 强制类型转换 输出string  [显示函数！]
{
  std::stringstream os;
  switch (attr_type_) {
    case INTS: {
      os << num_value_.int_value_;
    } break;
    case FLOATS: {
      os << common::double_to_str(num_value_.float_value_);
    } break;
    case DATES: {
      os << timestamp_to_string(num_value_.date_value_);
    } break;
    case BOOLEANS: {
      os << num_value_.bool_value_;
    } break;
    case CHARS: {
      os << str_value_;
    } break;
    default: {
      LOG_WARN("unsupported attr type: %d", attr_type_);
    } break;
  }
  return os.str();
}
int Value::compare(const Value &other) const  // 根据数据类型 调用不同的比较方法
{
  if (this->attr_type_ == other.attr_type_) {
    switch (this->attr_type_) {
      case INTS: {
        return common::compare_int((void *)&this->num_value_.int_value_, (void *)&other.num_value_.int_value_);
      } break;
      case FLOATS: {
        return common::compare_float((void *)&this->num_value_.float_value_, (void *)&other.num_value_.float_value_);
      } break;
      case DATES: {
        return common::compare_date((void *)&this->num_value_.date_value_, (void *)&other.num_value_.date_value_);
      } break;
      case CHARS: {
        return common::compare_string((void *)this->str_value_.c_str(),
            this->str_value_.length(),
            (void *)other.str_value_.c_str(),
            other.str_value_.length());
      } break;
      case BOOLEANS: {
        return common::compare_int((void *)&this->num_value_.bool_value_, (void *)&other.num_value_.bool_value_);
      }
      default: {
        LOG_WARN("unsupported type: %d", this->attr_type_);
      }
    }
  } else if (this->attr_type_ == INTS && other.attr_type_ == FLOATS) { // int float 强制类型转化后 再比较
    float this_data = this->num_value_.int_value_;
    return common::compare_float((void *)&this_data, (void *)&other.num_value_.float_value_);
  } else if (this->attr_type_ == FLOATS && other.attr_type_ == INTS) {
    float other_data = other.num_value_.int_value_;
    return common::compare_float((void *)&this->num_value_.float_value_, (void *)&other_data);
  } 
  // dates & ints
  else if (this->attr_type_ == DATES && other.attr_type_ == INTS) {
    return common::compare_int((void *)&this->num_value_.date_value_, (void *)&other.num_value_.int_value_);
  } else if (this->attr_type_ == INTS && other.attr_type_ == DATES) {
    return common::compare_int((void *)&this->num_value_.int_value_, (void *)&other.num_value_.date_value_);
  } 
  // dates & floats
  else if (this->attr_type_ ==  DATES && other.attr_type_ == FLOATS) {
    float this_data = this->num_value_.date_value_;
    return common::compare_float((void *)&this_data, (void *)&other.num_value_.float_value_);
  } else if (this->attr_type_ == FLOATS && other.attr_type_ == DATES) {
    float other_data = other.num_value_.date_value_;
    return common::compare_float((void *)&this->num_value_.float_value_, (void *)&other_data);
  }
  //  dates & chars 
  else if (this->attr_type_ == CHARS && other.attr_type_ == DATES){
    int this_data = chars_to_timestamp((this->str_value_).c_str());
    if(this_data == -1) return -1;  // date check
    else return common::compare_date((void *)&this_data, (void *)&other.num_value_.date_value_);
  } else if (this->attr_type_ ==  DATES && other.attr_type_ == CHARS){
    int other_data = chars_to_timestamp((other.str_value_).c_str());
    if(other_data == -1) return -1; //date check
    return common::compare_date((void *)&this->num_value_.date_value_, (void *)&other_data);
  }

  LOG_WARN("not supported");
  return -1;  // TODO return rc?
}

int Value::get_int() const
{
  switch (attr_type_) {
    case CHARS: {
      try {
        return (int)(std::stol(str_value_));
      } catch (std::exception const &ex) {
        LOG_TRACE("failed to convert string to number. s=%s, ex=%s", str_value_.c_str(), ex.what());
        return 0;
      }
    }
    case INTS: {
      return num_value_.int_value_;
    }
    case FLOATS: {
      return (int)(num_value_.float_value_);
    }
    case BOOLEANS: {
      return (int)(num_value_.bool_value_);
    }
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}
int Value::get_date() const
{
  switch (attr_type_) {
    case CHARS: {
      try {
        return chars_to_timestamp(str_value_.c_str());
      } catch (std::exception const &ex) {
        LOG_TRACE("failed to convert string to number. s=%s, ex=%s", str_value_.c_str(), ex.what());
        return 0;
      }
    }
    case INTS: {
      return num_value_.int_value_;
    }
    case DATES: {
      return num_value_.date_value_;
    }
    case FLOATS: {
      return (int)(num_value_.float_value_);
    }
    case BOOLEANS: {
      return (int)(num_value_.bool_value_);
    }
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}
float Value::get_float() const
{
  switch (attr_type_) {
    case CHARS: {
      try {
        return std::stof(str_value_);
      } catch (std::exception const &ex) {
        LOG_TRACE("failed to convert string to float. s=%s, ex=%s", str_value_.c_str(), ex.what());
        return 0.0;
      }
    } break;
    case INTS: {
      return float(num_value_.int_value_);
    } break;
    case FLOATS: {
      return num_value_.float_value_;
    } break;
    case BOOLEANS: {
      return float(num_value_.bool_value_);
    } break;
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}
std::string Value::get_string() const
{
  return this->to_string();
}
bool Value::get_boolean() const
{
  switch (attr_type_) {
    case CHARS: {
      try {
        float val = std::stof(str_value_);
        if (val >= EPSILON || val <= -EPSILON) {
          return true;
        }

        int int_val = std::stol(str_value_);
        if (int_val != 0) {
          return true;
        }

        return !str_value_.empty();
      } catch (std::exception const &ex) {
        LOG_TRACE("failed to convert string to float or integer. s=%s, ex=%s", str_value_.c_str(), ex.what());
        return !str_value_.empty();
      }
    } break;
    case INTS: {
      return num_value_.int_value_ != 0;
    } break;
    case FLOATS: {
      float val = num_value_.float_value_;
      return val >= EPSILON || val <= -EPSILON;
    } break;
    case BOOLEANS: {
      return num_value_.bool_value_;
    } break;
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return false;
    }
  }
  return false;
}

int chars_to_timestamp(const char *str_time) {
  struct tm stm;
  int iY, iM, iD;  // 暂时支持精确到day 时间默认为00:00:00
  memset(&stm, 0, sizeof(stm));

  int pos = 0; // 记录间隔符 - 位置
  int MAX_LENGTH = 20;

  iY = atoi(str_time);
  // 闰年判断
  int flag = 0;	// 该年不是闰年
	if ((iY % 4 == 0 && iY % 100 != 0) || iY % 400 == 0)
		flag = 1;   // 该年是闰年

  while(str_time[pos++] != '-' && pos < MAX_LENGTH);
  iM = atoi(str_time + pos);
  while(str_time[pos++] != '-' && pos < MAX_LENGTH);
  iD = atoi(str_time + pos);

  // 异常检测 [此时只是给出warning 并不能返回 RC为falure]
  int days[] = {0, 31, 28 + flag, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if(iM <= 0 || iM >= 13)                    {return -1; LOG_WARN("unsupported date : %s", str_time);}
  if(iD <= 0 || iD > days[iM])              {return -1; LOG_WARN("unsupported date : %s", str_time);}
  if(iY < 1970 || iY >= 2032 && iM > 2)     {return -1; LOG_WARN("unsupported date : %s", str_time);}

  stm.tm_year = iY - 1900;
  stm.tm_mon = iM - 1;
  stm.tm_mday = iD;

  return (int)mktime(&stm);
}
std::string timestamp_to_string(int stamp) {

    time_t timestamp = time_t(stamp); // 获取当前时间戳
    struct tm *tm_local = localtime(&timestamp); // 转换为本地时间
 
    char date_buffer[80]; // 存储日期字符串的数组

    strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d ", tm_local); // 格式化字符串
    std::string date = date_buffer;
    return date;
}
