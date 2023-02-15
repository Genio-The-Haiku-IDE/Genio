/*
 * Copyright 2023 D. Alfano
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef BEXCEPTIONS_H
#define BEXCEPTIONS_H

#include <Errors.h>

	class BException {
	public:
						BException(BString const& msg, status_t type, 
							status_t error)
							:
							_message(msg),
							_type(type),
							_error(error)
						{}

		BString			Message() noexcept { return _message; }
		status_t		Type() noexcept { return _type; }
		status_t		Error() noexcept { return _error; }
				
	private:
		BString			_message;
		status_t		_type;	
		status_t		_error;
	};
	
	
	class BGeneralException : BException {
	public:
						BGeneralException(BString const& msg, status_t error)
							: BException(msg, B_GENERAL_ERROR_BASE, error)
						{}
	};
	
	class BOSException : BException {
	public:
						BOSException(BString const& msg, status_t error)
							: BException(msg, B_OS_ERROR_BASE, error)
						{}
	};
	
	class BAppException : BException {
	public:
						BAppException(BString const& msg, status_t error)
							: BException(msg, B_APP_ERROR_BASE, error)
						{}
	};
	
	class BInterfaceException : BException {
	public:
						BInterfaceException(BString const& msg, status_t error)
							: BException(msg, B_INTERFACE_ERROR_BASE, error)
						{}
	};
	
	class BMediaException : BException {
	public:
						BMediaException(BString const& msg, status_t error)
							: BException(msg, B_MEDIA_ERROR_BASE, error)
						{}
	};
	
	class BTranslationException : BException {
	public:
						BTranslationException(BString const& msg, status_t error)
							: BException(msg, B_TRANSLATION_ERROR_BASE, error)
						{}
	};
	
	class BMidiException : BException {
	public:
						BMidiException(BString const& msg, status_t error)
							: BException(msg, B_MIDI_ERROR_BASE, error)
						{}
	};
	
	class BStorageException : BException {
	public:
						BStorageException(BString const& msg, status_t error)
							: BException(msg, B_STORAGE_ERROR_BASE, error)
						{}
	};
	
	class BPosixException : BException {
	public:
						BPosixException(BString const& msg, status_t error)
							: BException(msg, B_POSIX_ERROR_BASE, error)
						{}
	};
	
	class BMailException : BException {
	public:
						BMailException(BString const& msg, status_t error)
							: BException(msg, B_MAIL_ERROR_BASE, error)
						{}
	};
	
	class BPrintException : BException {
	public:
						BPrintException(BString const& msg, status_t error)
							: BException(msg, B_PRINT_ERROR_BASE, error)
						{}
	};
	
	class BDeviceException : BException {
	public:
						BDeviceException(BString const& msg, status_t error)
							: BException(msg, B_DEVICE_ERROR_BASE, error)
						{}
	};


#endif // BEXCEPTIONS_H
