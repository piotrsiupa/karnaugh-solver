#include "./IndentedOStream.hh"


void IndentedOStream::insertIndent()
{
	if (newLine)
	{
		for (std::size_t i = 0; i != indentSize; ++i)
			*o << options::indent.get();
		newLine = false;
	}
}

void IndentedOStream::printText(std::string_view text)
{
	while (!text.empty())
	{
		std::string_view::size_type n = text.find_first_of("\n\\\"");
		if (options::indent.hasIndentOnEmpty() || text.front() != '\n')
			insertIndent();
		if (n == std::string_view::npos)
		{
			*o << text;
			break;
		}
		else if (text[n] == '\n')
		{
			*o << text.substr(0, ++n);
			newLine = true;
		}
		else
		{
			*o << text.substr(0, n);
			if (sanitization)
				*o << '\\';
			*o << text[n++];
		}
		text.remove_prefix(n);
	}
}
