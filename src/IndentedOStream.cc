#include "./IndentedOStream.hh"

#include "options.hh"


void IndentedOStream::insertIndent()
{
	if (newLine)
	{
		for (std::size_t i = 0; i != indentSize; ++i)
			*o << options::indent.get();
		newLine = false;
	}
}

void IndentedOStream::printText(const std::string_view text)
{
	std::string_view::size_type i = 0;
	while (i != text.size())
	{
		insertIndent();
		std::string_view::size_type j = text.find_first_of("\n\\\"", i);
		if (j == std::string_view::npos)
		{
			*o << text.substr(i);
			break;
		}
		else if (text[j] == '\n')
		{
			*o << text.substr(i, ++j - i);
			newLine = true;
		}
		else
		{
			*o << text.substr(i, j - i);
			if (sanitization)
				*o << '\\';
			*o << text[j++];
		}
		i = j;
	}
}
