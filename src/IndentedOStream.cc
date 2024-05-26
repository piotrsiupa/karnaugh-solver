#include "./IndentedOStream.hh"


void IndentedOStream::insertIndent()
{
	if (newLine)
	{
		for (std::size_t i = 0; i != indentSize; ++i)
			o << '\t';
		newLine = false;
	}
}

void IndentedOStream::printText(const std::string_view text)
{
	std::string_view::size_type i = 0;
	while (i != text.size())
	{
		insertIndent();
		std::string_view::size_type j = text.find('\n', i);
		if (j == std::string_view::npos)
		{
			o << text.substr(i);
			break;
		}
		else
		{
			++j;
			o << text.substr(i, j - i);
			i = j;
			newLine = true;
		}
	}
}
