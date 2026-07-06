vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO CharmedBaryon/CommonLibSSE
  REF b93280e832f263dbef44e44cbe2936622a02f91a
  SHA512 c98a0dde8fab45d0b5ffc8241bc6437fc1b2855e5577e9b66fe4e237c35c7111c3288f0ef7a637b2a399dd38938221619331e3b49504b76ac0e1a0a2034715a6
  HEAD_REF dev-ng
)

vcpkg_replace_string("${SOURCE_PATH}/CMakeLists.txt"
  "option(BUILD_TESTS \"Enable building of the unit tests.\" ON)"
  "option(BUILD_TESTS \"Enable building of the unit tests.\" OFF)"
)

vcpkg_configure_cmake(
  SOURCE_PATH "${SOURCE_PATH}"
  PREFER_NINJA
  OPTIONS
    "-DRAPIDCSV_INCLUDE_DIRS=${CURRENT_INSTALLED_DIR}/include"
    "-DBUILD_TESTS=OFF"
)

vcpkg_install_cmake()
vcpkg_cmake_config_fixup(PACKAGE_NAME CommonLibSSE CONFIG_PATH lib/cmake)
vcpkg_copy_pdbs()

file(GLOB CMAKE_CONFIGS "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/CommonLibSSE/*.cmake")
file(INSTALL ${CMAKE_CONFIGS} DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")
file(INSTALL "${SOURCE_PATH}/cmake/CommonLibSSE.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/CommonLibSSE")

file(
  INSTALL "${SOURCE_PATH}/LICENSE"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright)
