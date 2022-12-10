#pragma once

inline void fatalError(const std::string_view message, const std::source_location sourceLocation = std::source_location::current())
{
	const std::string errorMessage =
		std::string(message.data() +
			std::format(" Source Location data : File Name -> {}, Function Name -> {}, Line Number -> {}, Column -> {}", sourceLocation.file_name(), sourceLocation.function_name(), sourceLocation.line(), sourceLocation.column()));

	throw std::runtime_error(errorMessage.data());
}

inline void throwIfFailed(const HRESULT hr, const std::source_location sourceLocation = std::source_location::current())
{
	if (FAILED(hr))
	{
		fatalError("HRESULT failed!", sourceLocation);
	}
}