#pragma once
#pragma execution_character_set("utf-8")
#include <vector>
#include <string>
#include <optional>
#include <functional>

// Error reporting callback type
using ErrorCallback = std::function<void(const std::string& message)>;

// FileParser - Utility class for safe file reading and parsing
// The program should never crash. Problems in files are overcome when possible,
// otherwise a proper message is provided to the user.
class FileParser {
public:
    // Default error handler (prints to stderr)
    static void defaultErrorHandler(const std::string& message);
    
    // Set global error handler
    static void setErrorHandler(ErrorCallback handler);
    
    // Report an error through the current handler
    static void reportError(const std::string& message);
    
    // Check if any errors occurred during parsing
    static bool hasErrors();
    
    // Clear error state
    static void clearErrors();
    
    // Get executable directory for resource loading
    static std::string getExeDirectory();
    
    // Find a file in common locations (exe dir, parent dir, current dir)
    static std::optional<std::string> findFile(const std::string& filename);
    
    // Read entire file content as string
    static std::optional<std::string> readFileContent(const std::string& filepath);
    
    // Read file as lines (handles BOM and different line endings)
    static std::vector<std::string> readFileLines(const std::string& filepath);
    
    // Safe integer parsing with default value
    static int parseInt(const std::string& str, int defaultValue = 0);
    
    // Safe character parsing
    static char parseChar(const std::string& str, char defaultValue = '\0');
    
    // Trim whitespace from string
    static std::string trim(const std::string& str);
    
    // Split string by delimiter
    static std::vector<std::string> split(const std::string& str, char delimiter);
    
    // Check if string starts with prefix
    static bool startsWith(const std::string& str, const std::string& prefix);
    
private:
    static ErrorCallback s_errorHandler;
    static bool s_hasErrors;
};
