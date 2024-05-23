#!python3
# Copyright 2024, The Genio Team
# All rights reserved. Distributed under the terms of the MIT license.
# Author: Nexus6 <nexus6@disroot.org>

import subprocess
from Be import BApplication, BMessenger, BMessage, int32, BAlert, \
    B_GET_PROPERTY, B_EXECUTE_PROPERTY, B_FUNCTION_KEY, B_F10_KEY

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
    subprocess.run(["Zeal", symbol])


class App(BApplication):
    def __init__(self):
        BApplication.__init__(self, "application/x-vnd.Genio.ShowDocumentation")

    def QuitRequested(self):
        print("Quitting")
        # self.UnegisterExtension()
        return True

    def ReadyToRun(self):
        # result, error = self.CheckExtension()
        # if (result is False):
            # result, error = self.RegisterExtension()
            # if (result is False):
                # self.QuitRequested()

    def MessageReceived(self, msg):
        if msg.what == int32(b'gnio'):
            alert = BAlert("Genio", "Message from Genio", "OK")
            alert.Go()
            selection, error = GetSelection()
            if selection != "" and error == 0:
                OpenZeal(selection)
            else:
                symbol, error = GetSymbol()
                if symbol != "" and error == 0:
                    OpenZeal(symbol)
        else:
            BApplication.MessageReceived(self, msg)

    # def CheckExtension(self):
        # message = BMessage(B_GET_PROPERTY)
        # message.AddSpecifier("RegisterExtension", "ShowDocumentation")
        # reply = BMessage()
        # genio.SendMessage(message, reply)
        # result = reply.GetBool("result", False)
        # error = reply.GetInt32("error", 0)
        # return result, error
#
    # def RegisterExtension(self):
        # message = BMessage(B_EXECUTE_PROPERTY)
        # message.AddSpecifier("RegisterExtension", "ShowDocumentation")
        # message.AddString("description", "Show documentation in Zeal")
        # message.AddInt32("modifier", B_FUNCTION_KEY)
        # message.AddByte("shortcut", B_F10_KEY)
        # message.AddString("type", "command")
        # message.AddString("context", "editor")
        # message.AddString("files", "*")
        # reply = BMessage()
        # genio.SendMessage(message, reply)
        # result = reply.GetBool("result", False)
        # error = reply.GetInt32("error", 0)
        # return result, error
#
    # def UnregisterExtension(self):
        # message = BMessage(B_EXECUTE_PROPERTY)
        # message.AddSpecifier("RegisterExtension", "ShowDocumentation")
        # genio.SendMessage(message, None)


def main():
    global be_app
    be_app = App()
    be_app.Run()


if __name__ == "__main__":
    main()
