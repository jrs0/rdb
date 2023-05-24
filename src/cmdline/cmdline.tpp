/**
 * \file cmdline.tpp
 * \brief Template definitions
 *
 */

/// \brief Add an option  associated to an integer variable
template<Integral T>
void CommandLine::addOption(char short_name, const std::string & long_name,
			    std::string desc) {
    if(desc != "") {
	desc += "; ";
    }
    desc += "argument type integer";
    
    storeOption(long_name, short_name, Argument::Yes, desc);
    
    /*
     * Lambda for writing integer to variable
     *
     * The function performs checks to make sure that the string returned by
     * getopt is the correct type and is in range. Then the string is written
     * to the integer variable var.
     *
     */
    auto writeInteger = [&](char opt, char * optarg) {
	try {
	    std::size_t pos{ 0 }; // To store length of int
	    std::size_t strlen{ std::strlen(optarg) };
	    // Get the variable
	    //T var = stringToInt<T>(optarg, &pos);
	    // Store the variable in the return_map
	    return_map.emplace(opt, stringToInt<T>(optarg, &pos));
	    if(pos != strlen) {
		std::string msg{ "invalid argument "
			+ std::string(optarg) + " for -"
			+ std::string(1, opt) + ", must be of type int\n" };
		logError(msg);
		return -1;
	    }
	} catch(const std::invalid_argument & e) {
	    std::string msg = "invalid argument "
	    + std::string(optarg) + " for -"
	    + std::string(1, opt) + ", must be of type int";
	    logError(msg);
	    return -1;
	} catch(const std::out_of_range & e) {
	    std::string msg = "integer argument "
	    + std::string(optarg) + " for -"
	    + std::string(1, opt) + " is out of range";
	    logError(msg);
	    return -1;
	}
	logOption(opt, optarg); // Log the event
	return 0;
    };

    // Store the lambda to execute later
    lambda_map[short_name] = writeInteger; 

}

/// \brief Add an option associated to a floating point variable
template<std::floating_point T>
void CommandLine::addOption(char short_name, const std::string & long_name,
			    std::string desc) {
	
    if(desc != "") {
	desc += "; ";
    }
    desc += "argument type double";
	
    storeOption(long_name, short_name, Argument::Yes, desc);

    /*
     * Lambda to write double to variable
     *
     * The function writes a double argument provided by getopt to the double
     * variable var. Checks are performed to make sure that the argument is the
     * right type for double and is in range.
     */
    auto writeDouble = [&](char opt, char * optarg) {
	try {
	    std::size_t pos{ 0 }; // To store length of double
	    std::size_t strlen{ std::strlen(optarg) };
	    // Store in the return map
	    return_map.emplace(opt, stringToFP<T>(optarg, &pos));
	    if(pos != strlen) {
		std::string msg{ "invalid argument "
			+ std::string(optarg) + " for -"
			+ std::string(1, opt) + ", must be of type double" };
		logError(msg);
		return -1;
	    }
	} catch(const std::invalid_argument & e) {
	    std::string msg = "invalid argument "
	    + std::string(optarg) + " for -"
	    + std::string(1, opt) + ", must be of type double";
	    logError(msg);
	    return -1;
	} catch(const std::out_of_range & e) {
	    std::string msg = "floating point (double) argument "
	    + std::string(optarg) + " for -"
	    + std::string(1, opt) + " is out of range";
	    logError(msg);
	    return -1;
	}
	logOption(opt, optarg); // Log the event
	return 0;
	
    };
    
    lambda_map[short_name] = writeDouble; // Store the lambda to execute later

}

template<typename T>
void CommandLine::addOption(char short_name, const std::string & long_name,
			    const std::map<std::string, T> & map,
			    std::string desc) {

    if(desc != "") {
	desc += "; ";
    }
    desc += "argument type string";
    
    storeOption(long_name, short_name, Argument::Yes, desc);

    /*
     * Lambda function for writing corresponding type to variable
     *
     * This function uses the key-value structure map to associate a
     * string with a variable of type T. If the user enters a valid
     * string in map, the corresponding variable of type T is stored in
     * var. If the user does not enter a valid string, a list of valid
     * options is printed instead.
     *
     */

    auto writeVariable = [&](char opt, char * optarg) {
	std::string arg{ optarg }; 

	// Check whether the option is in the list map.at()	    
	try {
	    // Store in the return map
	    return_map.emplace(opt, map.at(arg));
	} catch(const std::out_of_range & e) {
	    std::string msg = "invalid argument for -"
	    + std::string(1, opt);
	    msg += " (--" + options.getLongName(opt);
	    msg += "). Valid arguments are:";
	    for(auto it = std::begin(map); it != std::end(map); ++it) {
		std::string argument{ it->first };
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

template<typename T>
std::optional<T> CommandLine::get(char short_name) {
    try {
	return std::optional<T> {
	    std::any_cast<T>(return_map.at(short_name))
		};
    } catch(const std::out_of_range & e) {
	return std::optional<T>{std::nullopt};
    } catch(const std::bad_any_cast & e) {
	std::cout << std::endl
		  << "Exception: " << e.what()
		  << " while attempting to read command line argument "
		  << std::string(1, short_name) << "."
		  << std::endl
		  << "Check that the template parameter to get() "
		  << "matches the template parameter used with "
		  << "addOption()" << std::endl << std::endl;
	    
	// Abort, error due to programming problem
	abort();
    }
}
