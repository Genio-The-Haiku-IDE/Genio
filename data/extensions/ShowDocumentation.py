#!python3
# Copyright 2024, The Genio Team
# All rights reserved. Distributed under the terms of the MIT license.
# Author: Nexus6 <nexus6@disroot.org>

import subprocess
from Be import BMessenger, BMessage, B_GET_PROPERTY

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


def OpenZeal(symbol):
    subprocess.Popen(["Zeal", symbol])


def main():
    selection, error = GetSelection()
    if selection != "" and error == 0:
        OpenZeal(selection)
    else:
        symbol, error = GetSymbol()
        if symbol != "" and error == 0:
            OpenZeal(symbol)

    print("Terminato")


if __name__ == "__main__":
    main()
