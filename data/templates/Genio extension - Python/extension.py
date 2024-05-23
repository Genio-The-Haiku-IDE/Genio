#!python3
# Copyright 2024, Name
# All rights reserved. Distributed under the terms of the MIT license.
# Author: Name <name@domain>

from Be import BMessenger, BMessage, \
    B_GET_PROPERTY, B_SET_PROPERTY, B_EXECUTE_PROPERTY, B_COUNT_PROPERTIES, \
    B_CREATE_PROPERTY, B_DELETE_PROPERTY, B_GET_SUPPORTED_SUITES

genio = BMessenger("application/x-vnd.Genio")


def GetSymbol():
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Symbol")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetString("result", "")
    error = reply.GetInt32("error", 0)
    return result, error


def GetSelection():
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Selection")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetString("result", "")
    error = reply.GetInt32("error", 0)
    return result, error


def main():
    # your main starts here


if __name__ == "__main__":
    main()
