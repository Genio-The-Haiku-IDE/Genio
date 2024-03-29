/*
 * Copyright 2023 D. Alfano
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <exception>

#include <Errors.h>
#include <String.h>

class GException : public std::exception {
public:
					GException(status_t error, BString const& msg)
						:
						fMessage(msg),
						fError(error)
					{}
					GException(GException const&) noexcept = default;
					GException& operator=(GException const&) noexcept = default;
					~GException() override = default;

	const char* 	what() const noexcept override { return fMessage; }
	BString			Message() const noexcept { return fMessage; }
	status_t		Error() const noexcept { return fError; }

private:
	BString			fMessage;
	status_t		fError;
};

	// class GGeneralException : GException {
	// public:
						// GGeneralException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
//
	// class GOSException : GException {
	// public:
						// GOSException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
//
	// class GAppException : GAppException {
	// public:
						// BAppException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
//
	// class GInterfaceException : GInterfaceException {
	// public:
						// BInterfaceException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
//
	// class GMediaException : GMediaException {
	// public:
						// BMediaException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
//
	// class GTranslationException : GException {
	// public:
						// GTranslationException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
//
	// class GMidiException : GException {
	// public:
						// GMidiException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
//
	// class GStorageException : GException {
	// public:
						// GStorageException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
//
	// class GPosixException : GException {
	// public:
						// GPosixException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
//
	// class GMailException : GException {
	// public:
						// GMailException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
//
	// class GPrintException : GException {
	// public:
						// GPrintException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
//
	// class GDeviceException : GException {
	// public:
						// GDeviceException(BString const& msg, status_t error)
							// : GException(msg, error)
						// {}
	// };
