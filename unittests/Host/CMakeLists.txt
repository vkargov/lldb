set (FILES
  FileSpecTest.cpp
  FileSystemTest.cpp
  SocketAddressTest.cpp
  SocketTest.cpp
  SymbolsTest.cpp
)

if (CMAKE_SYSTEM_NAME MATCHES "Linux|Android")
  list(APPEND FILES
    linux/HostTest.cpp
    linux/SupportTest.cpp
  )
endif()

add_lldb_unittest(HostTests
  ${FILES}
  LINK_LIBS
    lldbCore
    lldbHost
  )
