#include "./options.hh"

#include <iostream>


namespace options
{
	
	bool Flag::parse(std::string_view argument)
	{
		if (!argument.empty())
		{
			std::cerr << argument << '\n';
			std::cerr << "The option \"--" << getLongName() << "\" doesn't take an argument!\n";
			return false;
		}
		raised = true;
		return true;
	}
	
	
	Flag help("help", 'h');
	Flag version("version");
	
	std::vector<std::string_view> freeOptions;
	
	
	static Option *const allOptions[] = {&help, &version};
	
	
	bool parse(const int argc, const char *const *const argv)
	{
		int i;
		for (i = 1; i != argc; ++i)
		{
			if (argv[i][0] != '-')
			{
				free_option:
				freeOptions.emplace_back(argv[i]);
			}
			else
			{
				if (argv[i][1] == '\0')
				{
					goto free_option;
				}
				else if (argv[i][1] != '-')
				{
					// Short options
					for (const char *shortOption = argv[i] + 1; *shortOption != '\0'; ++shortOption)
					{
						for (Option *const option : allOptions)
						{
							if (option->getShortName() == *shortOption)
							{
								if (option->needsArgument())
								{
									if (*(shortOption + 1) != '\0')
									{
										if (!option->parse(shortOption + 1))
											return false;
									}
									else if (++i != argc)
									{
										if (!option->parse(argv[i]))
											return false;
									}
									else
									{
										std::cerr << "The option \"" << option->getLongName() << "\" requires an argument!\n";
										return false;
									}
								}
								else
								{
									if (!option->parse(""))
										return false;
								}
								goto next_short_option;
							}
						}
						std::cerr << "Unknown option \"-" << *shortOption << "\"!\n";
						return false;
						next_short_option:;
					}
				}
				else
				{
					if (argv[i][2] == '\0')
					{
						++i;
						break;
					}
					else
					{
						// Long option
						std::string_view longOption = argv[i] + 2, argument = "";
						if (const std::string_view::size_type pos = longOption.find('='); pos != std::string_view::npos)
						{
							argument = longOption.substr(pos + 1);
							longOption = longOption.substr(0, pos);
						}
						for (Option *const option : allOptions)
						{
							if (option->getLongName() == longOption)
							{
								if (option->needsArgument() && argument.empty())
								{
									if (++i == argc)
									{
										std::cerr << "The option \"" << option->getLongName() << "\" requires an argument!\n";
										return false;
									}
								}
								if (!option->parse(argument))
									return false;
								goto next_arg;
							}
						}
						std::cerr << "Unknown option \"--" << longOption << "\"!\n";
						return false;
					}
				}
			}
			next_arg:;
		}
		// Free options after "--"
		for (; i != argc; ++i)
			freeOptions.emplace_back(argv[i]);
		return true;
	}
	
}
