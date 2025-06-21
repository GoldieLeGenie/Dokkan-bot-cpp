cmake_minimum_required(VERSION 3.8)
project(dokkanCPP)

# === Options de compilation ===
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# === Choix du build type (Release par défaut) ===
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()


# === Trouver les dépendances ===
find_package(OpenSSL REQUIRED)
find_package(sqlcipher REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(cpr REQUIRED)

# === Fichiers sources et headers ===
file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Database/pysqlsimplecipher/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Database/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Cryption/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Api/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Other/*.cpp
)

file(GLOB_RECURSE HEADERS 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Database/pysqlsimplecipher/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Database/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Cryption/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Api/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Other/*.hpp
)

# === Création de l'exécutable ===
add_executable(dokkanCPP ${SOURCES} ${HEADERS})

# === Include directories ===
target_include_directories(dokkanCPP 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Database
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Api
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Cryption
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Other
        ${OPENSSL_INCLUDE_DIR}
)

# === Link des bibliothèques ===
target_link_libraries(dokkanCPP
    PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        sqlcipher::sqlcipher
        nlohmann_json::nlohmann_json
        cpr::cpr
)

