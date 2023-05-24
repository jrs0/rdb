/**
 * \file cmdline.hpp
 * \brief Contains a command line option class based on getopt
 *
 *
 */

#ifndef CMDLINE_HPP
#define CMDLINE_HPP

#include <vector>
#include <map>
#include <getopt.h>
#include <cstring>
#include <functional>
#include <iostream>
#include <type_traits>
#include <concepts>
#include <any>
#include <optional>

// Used to check for string type
template <class T>
concept String = std::is_same<T,std::string>::value;

// Used to check for bool type
template <class T>
concept Bool = std::is_same<T,bool>::value;

// Used to check for integral type (stops treating bool as std::integral)
///\todo Fix this properly
template <class T>
concept Integral = std::is_integral<T>::value && !std::is_same<T,bool>::value;

/**
 * \brief Convert string to every floating point type
 */
template<std::floating_point T>
T stringToFP(std::string s, std::size_t * pos);

/**
 * \brief Convert string to every integral point type
 */
template<Integral T>
T stringToInt(std::string s, std::size_t * pos);

/// An enum to store whether there is an argument or not
enum class Argument {
    Yes = required_argument,
    No = no_argument,
};

/**
 * \brief A structure to hold the getopt option structure
 *
 *
 */
class Options {
private:
    std::vector<struct option> option_list; ///< The getopt internal options list
    std::map<char, std::string> names; ///< Map from short names to long names
    std::map<char, std::string> descriptions; ///< Map from short names to descriptions

    /// Store C strings in std::vector (to avoid new/delete)
    std::vector<std::vector<char>> c_strings;
        
public:

    void store(const std::string & long_name, char short_name, Argument arg,
	       const std::string & desc) {

	// Store the long and short name
	names[short_name] = long_name;
	descriptions[short_name] = desc;
	
	// Make option struct
	int arg_int = static_cast<int>(arg); // Need explicit conversion

	// Create a c-style string that can be pointed to by the
	// option struct.
        c_strings.emplace_back(std::begin(long_name), std::end(long_name)); 

	// Add the null-terminator to the end
	c_strings.back().push_back('\0');
	
	option opt = {&c_strings.back()[0], arg_int, 0, short_name};    
	option_list.push_back(opt);
    }
    
    /**
     * \brief Get pointer to start of getopt options list
     *
     * This function should only be called after all the options have been
     * added. The returned list has a null terminator 
     *
     */
    std::vector<struct option> getOptionArray() const {

	// Make a copy of the option list
	std::vector<struct option> result{ option_list };
	
	result.push_back({0,0,0,0}); // Add a null terminator
	return result;
    }

    std::string getLongName(char short_name) const {
	return names.at(short_name);
    }

    std::string getDescription(char short_name) const {
	return descriptions.at(short_name);
    }

    
};

/**
 * \brief A command line class based on the getopt C library
 *
 * The main feature of the design is the use of lambda functions
 * to write option arguments to variables specified by the user.
 * The reason for using lambda functions is to capture a reference 
 * to a variable when the user defines the option, and then writing
 * to that variable later after the command line options have been
 * parsed.
 */
class CommandLine {
private:

    std::string optstring;
    Options options;
    
    /**
     * The point of this variable is to map a short form argument to a lambda
     * function (stored as a function pointer) which writes a value to the 
     * corresponding option variable (using getopt data as arguments if 
     * necessary).
     *
     * The lambda function takes the opt and optarg variables from getopt. 
     * opt is used to print error messages, if there are any, and
     * optarg contains the content to be written to the variable.
     *
     * The option variable is captured by reference, and does not 
     * appear in the argument list, hence it does not affect the
     * function prototype (information about the type of variable
     * being written to has been removed). This avoids the need to store
     * references to different types of option variables directly, which
     * would cause type problems.
     *
     */
    std::map<char, std::function<int(char opt,char* optarg)>> lambda_map;

    std::map<char, std::any> return_map;
    
    /// Set up the option
    void addHelp();
    
    /// Add to the options list
    void storeOption(std::string long_name, char short_name,
		     Argument arg, std::string desc);
        
    /**
     * \brief Print option input
     *
     * This function can be used to print a summary line corresponding to
     * the option that is currently being parsed. 
     * 
     */
    void logOption(char opt, char * optarg);
    void logError(std::string msg);    
    
    std::vector<std::string> log; ///< Log from the parsing process
    std::vector<std::string> errors; ///< Errors from the parsing process

    void printLog();
    void printErrors(std::string program_name);

    enum class Type {
	Integer,
	Float,
	String
    };	
    
public:

    ///
    void setupManPageOption(const std::string & program_name,
			    const std::string & short_desc,
			    const std::string & long_desc,
			    const std::string & version);
    
    /**
     * \brief Constructor
     *
     */
    CommandLine();

    /**
     * \brief Add an option  associated to an integer variable
     *
     * This function works for any input type T which satisfies
     * std::integral. This includes int, long int, unsigned int,
     * etc. (it also includes std::size_t which resolves to some
     * integral type).
     *
     */
    template<Integral T>
    void addOption(char short_name, const std::string & long_name,
		   std::string desc);

    /**
     * \brief Add an option associated to a floating point variable
     *
     * This function works for any input type T which satisfies 
     * std::floating_point, which includes float, double and long double.
     *
     */
    template<std::floating_point T>
    void addOption(char short_name, const std::string & long_name,
		   std::string desc);
    
    /**
     * \brief Add an option associated to a map from a group of strings
     *
     * Use this function for options whose argument must be one string
     * from a list of predefined options. To use this function, first
     * decide on a type T whose value will be chosen based on a string
     * (for example a scoped enum, or an integer). Then make a map from
     * strings to T which associate user string inputs with values of
     * type T. Pass this map to the function. When the command line is
     * parsed, the variable var will be assigned a value depending which
     * string was entered.
     *
     * Checks are performed to make sure a valid string has been entered.
     *
     */
    template<typename T>
    void addOption(char short_name, const std::string & long_name,
		   const std::map<std::string, T> & map,
		   std::string desc);

    /**
     * \brief Add an option for a group of strings
     *
     * This function is like the map version above, but it just
     * returns the string inputted by the user
     *
     */
    void addOption(char short_name, const std::string & long_name,
		   const std::vector<std::string> & options,
		   std::string desc);
    
    
    /**
     * \brief Add an option associated to a string variable
     *
     * Use this function to add an option associated to an unrestricted
     * string variable (e.g. a filename). No checking is performed on the
     * string entered by the user.
     *
     */
    template<String T>
    void addOption(char short_name, const std::string & long_name,
		   std::string desc);

    /** 
     * \brief Add an option associated to a boolean variable 
     *
     * Use this function to associate an option with a boolean flag which
     * is set to true or false depending on whether the option is passed 
     * on the command line.
     *
     */
    template<Bool T>
    void addOption(char short_name, const std::string & long_name,
		   std::string desc);

    /**
     * \brief Parse the command line
     *
     * Use this function to parse the command line arguments, using argc and
     * argv from main. The prototype is the same as main: argc is an integer
     * containing the number of arguments and argv is a pointer to the start
     * of an array of c style strings (a pointer to char * is char **).
     *
     *
     * \return Returns 0 if there were no errors, and -1 if an error occured
     */
    int parse(int argc, char ** argv);

    /**
     * \brief Get the returned values
     *
     *
     */
    template<typename T>
    std::optional<T> get(char short_name);
    
};

#include "cmdline.tpp"

#endif
