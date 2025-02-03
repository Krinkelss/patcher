#pragma once

class WindowsException
{
public:
	WindowsException( const std::wstring& message, DWORD errorCode ) : message( message ), errorCode( errorCode )
	{}

	std::wstring getMessage() const
	{
		return message;
	}

	DWORD getErrorCode() const
	{
		return errorCode;
	}

private:
	std::wstring message;
	DWORD errorCode;
};