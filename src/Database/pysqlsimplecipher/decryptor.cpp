#include <iostream>
#include <sqlcipher/sqlite3.h>
#include <filesystem>  

int decrypt_database(const char* encrypted_db) {

    const char* decrypted_db = "glb.db";
    const char* key = "9bf9c6ed9d537c399a6c4513e92ab24717e1a488381e3338593abd923fc8a13b";

    // Delete glb.db if it already exists
    if (std::filesystem::exists(decrypted_db)) {
        try {
            std::filesystem::remove(decrypted_db);
            std::cout << "[+] Existing glb.db removed.\n";
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[!] Failed to delete glb.db: " << e.what() << "\n";
            return 1;
        }
    }

    sqlite3* db = nullptr;

    // Open the SQLCipher database
    if (sqlite3_open(encrypted_db, &db) != SQLITE_OK) {
        std::cerr << "[!] Failed to open database: " << sqlite3_errmsg(db) << "\n";
        return 1;
    }

    // Apply the decryption key
    std::string keyPragma = "PRAGMA key = '" + std::string(key) + "';";
    if (sqlite3_exec(db, keyPragma.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "[!] Failed to apply key.\n";
        sqlite3_close(db);
        return 1;
    }

    // SQLCipher compatibility (optional)
    sqlite3_exec(db, "PRAGMA cipher_compatibility = 3;", nullptr, nullptr, nullptr);

    // Attach a new unencrypted database
    std::string attach = "ATTACH DATABASE '" + std::string(decrypted_db) + "' AS decrypted KEY '';";
    if (sqlite3_exec(db, attach.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "[!] Failed to attach target database.\n";
        sqlite3_close(db);
        return 1;
    }

    // Export the content
    if (sqlite3_exec(db, "SELECT sqlcipher_export('decrypted');", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "[!] Failed to export data.\n";
        sqlite3_close(db);
        return 1;
    }

    // Detach the database
    if (sqlite3_exec(db, "DETACH DATABASE decrypted;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "[!] Failed to detach target database.\n";
        sqlite3_close(db);
        return 1;
    }

    std::cout << "[+] Database successfully decrypted to " << decrypted_db << "\n";
    sqlite3_close(db);
    return 0;
}
