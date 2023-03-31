/**
 * \file cmdline.cpp
 * \brief Implementation of command line option class
 *
 */

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <iostream>
#include <getopt.h>
#include <cstring>
#include "cmdline.hpp"
#include <fstream>
#include <ctime>
#include <stdexcept>

template<>
float stringToFP<float>(std::string s, std::size_t * pos) {
    return std::stof(s, pos);
}

template<>
double stringToFP<double>(std::string s, std::size_t * pos) {
    return std::stod(s, pos);
}

template<>
long double stringToFP<long double>(std::string s, std::size_t * pos) {
    return std::stold(s, pos);
}

template<>
int stringToInt<int>(std::string s, std::size_t * pos) {
    return std::stoi(s, pos);
}

template<>
long stringToInt<long>(std::string s, std::size_t * pos) {
    return std::stol(s, pos);
}

template<>
long long stringToInt<long long>(std::string s, std::size_t * pos) {
    return std::stoll(s, pos);
}

///\todo Check this properly
template<>
unsigned int stringToInt<unsigned int>(std::string s, std::size_t * pos) {
    
    unsigned long lresult = std::stoul(s, pos, 10);
    unsigned int result = lresult;
    if (result != lresult) {
	throw std::out_of_range("too big for unsigned int");
    }
    return result;
}

template<>
unsigned long stringToInt<unsigned long>(std::string s, std::size_t * pos) {
    return std::stoul(s, pos);
}

template<>
unsigned long long stringToInt<unsigned long long>(std::string s, std::size_t * pos) {
    return std::stoll(s, pos);
}


/**
 * \brief Get date in the format 
 */
std::string getDateString() {

    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];
    
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer,sizeof(buffer),"%d %B %Y",timeinfo);
    
    return std::string(buffer);
}


/// Add to the options list
void CommandLine::storeOption(std::string long_name, char short_name,
			      Argument arg, std::string desc) {
    
    optstring += short_name; // Store the short form
    if(arg == Argument::Yes) {
	optstring += ":"; // Specify that an argument is required
    }
    
    // Store the getopt structures and details
    options.store(long_name, short_name, arg, desc);
}


/**
 * \brief Print option input
 * 
 */
void CommandLine::logOption(char opt, char * optarg) {

    std::string entry{ "  -" + std::string(1, opt) };
    try {
	std::string long_name = options.getLongName(opt);
	entry += ", --" + long_name;
    } catch(const std::out_of_range & e) {
	// Do something here?
    }
    if(optarg != nullptr) {
	entry += " = " + std::string(optarg);
    } else {
	entry += " = true";
    }
    
    log.push_back(entry);
}

void CommandLine::logError(std::string msg) {
    errors.push_back(msg);
}

void CommandLine::printLog() {

    if(log.size() > 0) {
	std::cout << "Valid command line options:" << std::endl;
	for(std::size_t n = 0; n<log.size(); n++) {
	    std::cout << "  " << log[n] << std::endl;
	}
    }
}

void CommandLine::printErrors(std::string program_name) {
    if(errors.size() != 0) {
	for(std::size_t n = 0; n<errors.size(); n++) {
	    std::cout << "Error: " << errors[n] << std::endl;
	}
	std::cout << "Try '" << program_name << " --help' for more information."
		  << std::endl;
    }
}

void CommandLine::setupManPageOption(const std::string & program_name,
				     const std::string & short_desc,
				     const std::string & long_desc,
				     const std::string & version) {

    storeOption("export-man-page", 'x', Argument::No,
		"export man page for this program");    
    
    /*
     * Lambda function for exporting to man page
     *
     * The unused attribute is to prevent GCC from generating warnings because
     * opt and optarg aren't used. They have to be present in the function
     * definition because the standard vector of functions only accepts functions
     * which take these two arguments. 
     *
     */
    auto exportManPage = [=, this](__attribute__ ((unused)) char opt,
			     __attribute__ ((unused)) char * optarg)
	-> int {

	std::string filename{ program_name + ".man" };
	
	std::ofstream f;
	f.open(filename);

	// Print the header line
	f << ".TH " << program_name
	<< " 1 \" " << getDateString()
	<< "\" \"version " << version
	<< "\" "<< std::endl;

	// Print NAME section 
	f << std::endl << ".SH NAME" << std::endl;
	f << program_name << " - " << short_desc << std::endl;

	// Print DESCRIPTION section
	f << std::endl << ".SH DESCRIPTION" << std::endl;
	f << long_desc << std::endl;

	// Print OPTIONS section
	f << std::endl << ".SH OPTIONS" << std::endl;


	// Iterate over options
	std::vector<struct option> option_list{ options.getOptionArray() };	
	for(std::size_t n=0; n<option_list.size()-1; n++) {
	    option opt{ option_list[n] };

	    char short_name{ (char)opt.val };
	    std::string long_name{ options.getLongName(short_name) };
	    std::string description{ options.getDescription(short_name) };

	    // Use tag paragraph 
	    f << std::endl << ".TP" << std::endl;

	    // Write a line like: .B "-x, --long-form"
	    f << ".B \"-" + std::string(1, short_name)
		+ ", --" + long_name + "\"" << std::endl;
	    f << description << std::endl;
	    
	}

	f.close();

	std::cout << "Man page written to " << filename << std::endl
	<< "Run " << std::endl << "\t'man ./" << filename << "'"
	<< std::endl << std::endl
	<< "to view the man page."
	<< std::endl;
	exit(0);
    };
    
    lambda_map['x'] = exportManPage; // Store the lambda to execute later
	
}

void CommandLine::addHelp() {

    storeOption("help", 'h', Argument::No, "display this help and exit");    
    
    /*
     * Lambda function for providing help
     *
     * The unused attribute is to prevent GCC from generating warnings because
     * opt and optarg aren't used. They have to be present in the function
     * definition because the standard vector of functions only accepts functions
     * which take these two arguments. 
     *
     */
    auto provideHelp = [&]([[maybe_unused]] char opt,
			   [[maybe_unused]] char * optarg) -> int {
	
	
	// Iterate over options
	std::vector<struct option> option_list{ options.getOptionArray() };
	std::cout << "OPTIONS" << std::endl;
	
	for(std::size_t n=0; n<option_list.size()-1; n++) {
	    option opt{ option_list[n] };
	    std::cout << "    -" << (char)opt.val;
	    std::cout << ", --" << opt.name << std::endl;
	    std::cout << "      " << options.getDescription((char)opt.val)
		      << std::endl << std::endl;
	}
	std::cout << std::endl;
	exit(0); // Leave after printing help
    };
    
    lambda_map['h'] = provideHelp; // Store the lambda to execute later

}
    
/**
 * \brief Constructor
 *
 */
CommandLine::CommandLine() : optstring{"-:"}
{
    // Add a help command line option
    addHelp();
}
    
/// \brief Add an argument associated to a double variable

int CommandLine::parse(int argc, char ** argv)
{
    // Re-initialise getopt
    ///\todo This is a bad idea -- what about multiple
    /// instances of CommandLine, etc? Need to fix.
    optind = 1;
    
    // Variable for the short form of current option in while loop below
    int opt{ 0 };
    int index{ 0 };

    // Return value
    int code{ 0 };

    std::vector<struct option> options_array = options.getOptionArray();

    while((opt = getopt_long(argc, argv, optstring.c_str(),
			     &options_array[0], &index)) != -1) {

	switch(opt) {
	case '?': {
	    // Unrecognised cmdline option
	    std::string msg{"unrecognised command line option: "};
	    // Check for short or long form problem
	    if(optopt != 0) {
		// Short for problem
		msg += "-" + std::string(1,optopt);
	    } else {
		// Long form problem
		msg += std::string(argv[optind - 1]);
	    }
	    logError(msg);
	    code = -1; // Mark error flag
	    break;
	}
	case ':': {
	    // Option requires an argument
	    std::string msg{ "command line option -" + std::string(1, optopt) };
	    msg += " (--" + options.getLongName(optopt);
	    msg += ") requires an argument";
	    logError(msg);
	    code = -1; // Mark error flag
	    break;
	}
	case 1: {
	    // Non-option argument was provided
	    std::string msg{ "non-option argument " + std::string(optarg) };
	    msg += " was provided";
	    logError(msg);
	    code = -1; // Mark error flag
	    break;
	}
	default:
	    // Option is recognised correctly
	    try {
		// Check that the argument is in the map
		lambda_map.at(opt);
		//std::cout << "Option -" << (char)opt
		//	      << " was provided with argument "
		//	      << optarg << std::endl;
	    } catch (const std::out_of_range & e) {
		// This shouldn't happen (caught above in opt == ?)
		std::cerr << "Error: unexpected out-of-range "
			  << "(bug, see source code)"
			  << std::endl;
		abort(); // May as well not continue here
		break;
	    }		

	    // Now process the argument by executing the lambda
	    auto process_argument{ lambda_map[opt] };
	    if(process_argument(opt, optarg) != 0) {
		// Something went wrong parsing the optarg string
		code = -1;
	    }
	}
    }

    //printLog();
    printErrors(std::string(argv[0]));
    return code;
}

/// \brief Add an option associated to a string variable
template<>
void CommandLine::addOption<std::string>(char short_name,
					 const std::string & long_name,
					 std::string desc) {
    
    if(desc != "") {
	desc += "; ";
    }
    desc += "argument type string";
    
    storeOption(long_name, short_name, Argument::Yes, desc);

    /* 
     * Lambda to store string in variable
     *
     * Store a string passed from getopt in the variable var. At the moment 
     * no checks are performed on the string.
     *
     */
    auto writeString = [&](char opt, char * optarg) {
	// Store std::string in the return map
	return_map.emplace(opt, std::string(optarg));

	logOption(opt, optarg); // Log the event
	return 0;
    };
    
    lambda_map[short_name] = writeString; // Store the lambda to execute later
	
}

/// \brief Add an option associated to a boolean variable
template<>
void CommandLine::addOption<bool>(char short_name, const std::string & long_name,
				  std::string desc) {
    
    if(desc != "") {
	desc += "; ";
    }
    desc += "no argument";
    
    storeOption(long_name, short_name, Argument::No, desc);
    
    // Lambda to write boolean flag to file
    auto writeFlag = [&](char opt, char * optarg) {
			 
	// Store bool in the return map
	return_map.emplace(opt, true);
	
	logOption(opt, optarg); // Log the event
	return 0;
    };
    
    lambda_map[short_name] = writeFlag; // Store the lambda to execute later
    
    
}

void CommandLine::addOption(char short_name, const std::string & long_name,
			    const std::vector<std::string> & list,
			    std::string desc) {

    if(desc != "") {
	desc += "; ";
    }
    desc += "argument type string";
    
    storeOption(long_name, short_name, Argument::Yes, desc);

    /*
     * Lambda function for writing corresponding type to variable
     *
     * If the user does not enter a valid string, a list of valid
     * options is printed instead.
     *
     */
    auto writeVariable = [&](char opt, char * optarg) {
	std::string arg{ optarg }; 

	// Store in the return map
	auto pos = std::find(std::begin(list),
			     std::end(list),
			     arg); 
	if(pos != std::end(list)) {
	    return_map.emplace(opt, arg);
	} else {
	    std::string msg = "invalid argument for -"
	    + std::string(1, opt);
	    msg += " (--" + options.getLongName(opt);
	    msg += "). Valid arguments are:";
	    for(auto it = std::begin(list); it != std::end(list); ++it) {
		std::string argument{ *it };
		msg += "\n\t" + argument;
	    }
	    logError(msg);
	    return -1;
	}
	logOption(opt, optarg);
	return 0;
    };
    lambda_map[short_name] = writeVariable;	
}
