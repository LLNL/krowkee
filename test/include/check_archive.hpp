// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <krowkee/util/check.hpp>

#if __has_include(<cereal/cereal.hpp>)
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>

template <typename OArchiveType, typename IArchiveType, typename T>
bool ss_archive_test(const T &obj1) {
  std::stringstream os;
  {
    OArchiveType oarchive(os);
    oarchive(obj1);
  }
  std::string       archive_str = os.str();
  std::stringstream is(archive_str);
  T                 obj2;
  {
    IArchiveType iarchive(is);
    iarchive(obj2);
  }
  return obj1 == obj2;
}

template <typename T>
bool ss_bin_archive_test(const T &obj) {
  return ss_archive_test<cereal::BinaryOutputArchive,
                         cereal::BinaryInputArchive>(obj);
}
template <typename T>
bool ss_json_archive_test(const T &obj) {
  return ss_archive_test<cereal::JSONOutputArchive, cereal::JSONInputArchive>(
      obj);
}

template <typename FuncType, typename T>
void CHECK_ARCHIVE(const FuncType &func, const T &obj, const std::string &name,
                   const std::string &msg) {
  bool              success = func(obj);
  std::stringstream ss;
  ss << msg << " " << name;
  CHECK_CONDITION(success == true, ss.str());
}

template <typename T>
void CHECK_ALL_ARCHIVES(const T &obj, const std::string &msg) {
  CHECK_ARCHIVE(ss_bin_archive_test<T>, obj, "binary cereal archive", msg);
  CHECK_ARCHIVE(ss_json_archive_test<T>, obj, "JSON cereal archive", msg);
}
#endif