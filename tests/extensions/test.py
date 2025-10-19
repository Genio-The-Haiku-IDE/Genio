#!python3
# Copyright 2024, The Genio Team
# All rights reserved. Distributed under the terms of the MIT license.
# Author: Nexus6 <nexus6@disroot.org>

import time
from Be import BMessenger, BMessage, BEntry, BPath, entry_ref, \
    B_EXECUTE_PROPERTY, B_GET_PROPERTY, B_SET_PROPERTY, B_CREATE_PROPERTY, B_COUNT_PROPERTIES

genio = BMessenger("application/x-vnd.Genio")


def OpenEditor():
    file = "./test_file"
    entry = BEntry(file)
    path = BPath()
    entry.GetPath(path)
    message = BMessage(B_CREATE_PROPERTY)
    message.AddSpecifier("Editor", path.Path())
    reply = BMessage()
    genio.SendMessage(message, reply)

    # Check if the reply indicates success (no error field means success)
    error = reply.GetInt32("error", 0)
    # Also check that it's not a B_MESSAGE_NOT_UNDERSTOOD reply
    is_error_reply = (reply.what == 0x42_4d_4e_55)  # 'BMNU' - B_MESSAGE_NOT_UNDERSTOOD

    if error == 0 and not is_error_reply:
        print(f"{file} opened in new Editor. Test *OpenEditor* PASSED!")
    else:
        print(f"Test *OpenEditor* FAILED! error: {error}, what: {hex(reply.what)}")
        exit(0)


def Selection():
    # Test selecting text from line 1: "CHAPTER I. Down the Rabbit-Hole"
    # Select characters at offset 0-9 to get "CHAPTER I"
    testString = "CHAPTER I"
    SetSelection(0, 9)
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Selection")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetString("result", "")
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *Selection* FAILED! Error code: {error}")
        return False

    if result == testString:
        print(f"test *Selection* PASSED! Selected: '{result}'")
        return True
    else:
        print(f"test *Selection* FAILED! Expected: '{testString}', Got: '{result}'")
        return False


def SetSelection(index, length):
    message = BMessage(B_SET_PROPERTY)
    message.AddSpecifier("Selection", index, length)
    message.AddSpecifier("SelectedEditor")
    genio.SendMessage(message, None)


def GetLine(lineNumber):
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Line", lineNumber)
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetString("result", "")
    error = reply.GetInt32("error", 0)
    return result, error


def Line():
    # Test setting and getting a line - we'll modify line 3 (originally "Alice was beginning...")
    testString = "Alice was testing Genio's scripting capabilities!\n"
    message = BMessage(B_SET_PROPERTY)
    message.AddSpecifier("Line", 2)  # 0-indexed, so line 3
    message.AddSpecifier("SelectedEditor")
    message.AddString("data", testString)
    reply = BMessage()
    genio.SendMessage(message, reply)
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *Line* FAILED! Error setting line: {error}")
        return False

    result, error = GetLine(2)

    if error != 0:
        print(f"test *Line* FAILED! Error getting line: {error}")
        return False

    if result == testString:
        print(f"test *Line* PASSED! Line content matches")
        return True
    else:
        print(f"test *Line* FAILED! Expected: '{testString}', Got: '{result}'")
        return False


def CountLines():
    message = BMessage(B_COUNT_PROPERTIES)
    message.AddSpecifier("Line")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetInt32("result", -1)
    return result


def Insert():
    # Insert text at the beginning of the document
    testString = "[GENIO TEST] "
    insertPos = 0

    message = BMessage(B_SET_PROPERTY)
    message.AddSpecifier("Text", insertPos)
    message.AddSpecifier("SelectedEditor")
    message.AddString("data", testString)
    reply = BMessage()
    genio.SendMessage(message, reply)
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *Insert* FAILED! Error inserting text: {error}")
        return False

    # Verify insertion by selecting the inserted text
    SetSelection(insertPos, len(testString))

    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Selection")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetString("result", "")
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *Insert* FAILED! Error verifying insertion: {error}")
        return False

    if result == testString:
        print(f"test *Insert* PASSED! Inserted text verified: '{result}'")
        return True
    else:
        print(f"test *Insert* FAILED! Expected: '{testString}', Got: '{result}'")
        return False


def Append():
    # Append a new line to the end of the document
    testString = "-- End of Alice excerpt --"
    newLine = "\n" + testString

    message = BMessage(B_SET_PROPERTY)
    message.AddSpecifier("Text")
    message.AddSpecifier("SelectedEditor")
    message.AddString("data", newLine)
    reply = BMessage()
    genio.SendMessage(message, reply)
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *Append* FAILED! Error appending text: {error}")
        return False

    lineCount = CountLines()
    if lineCount < 0:
        print(f"test *Append* FAILED! Could not count lines")
        return False

    result, error = GetLine(lineCount - 1)

    if error != 0:
        print(f"test *Append* FAILED! Error getting appended line: {error}")
        return False

    if result == testString:
        print(f"test *Append* PASSED! Line appended successfully")
        return True
    else:
        print(f"test *Append* FAILED! Expected: '{testString}', Got: '{result}'")
        return False


def Undo():
    message = BMessage(B_EXECUTE_PROPERTY)
    message.AddSpecifier("Undo")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    error = reply.GetInt32("error", 0)

    if error == 0:
        print("test *Undo* PASSED! Changes undone successfully")
        return True
    else:
        print(f"test *Undo* FAILED! Error: {error}")
        return False


def Ref():
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Ref")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *Ref* FAILED! Error: {error}")
        return False

    ref = entry_ref()
    rez = reply.FindRef("result", ref)

    if rez == 0:
        fullpath = BPath(ref).Path()
        # Verify the path contains our test file
        if "test_file" in fullpath:
            print(f"test *Ref* PASSED! Path: {fullpath}")
            return True
        else:
            print(f"test *Ref* FAILED! Unexpected path: {fullpath}")
            return False
    else:
        print(f"test *Ref* FAILED! FindRef returned: {rez}")
        return False


def CaretPosition():
    # Set selection to position the caret at a known location
    testOffset = 50
    SetSelection(testOffset, 0)

    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("CaretPosition")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)

    caretInfo = BMessage()
    rez = reply.FindMessage("result", caretInfo)
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *CaretPosition* FAILED! Error: {error}")
        return False

    if rez == 0:
        line = caretInfo.GetInt32("line", -1)
        column = caretInfo.GetInt32("column", -1)
        offset = caretInfo.GetInt32("offset", -1)

        # Verify that we got valid values (all should be >= 0)
        if line > 0 and column > 0 and offset >= 0:
            print(f"test *CaretPosition* PASSED! Line: {line}, Column: {column}, Offset: {offset}")
            return True
        else:
            print(f"test *CaretPosition* FAILED! Invalid values - Line: {line}, Column: {column}, Offset: {offset}")
            return False
    else:
        print(f"test *CaretPosition* FAILED! FindMessage returned: {rez}")
        return False


def SelectionRange():
    # Set a known selection range
    testStart = 10
    testLength = 20
    SetSelection(testStart, testLength)

    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("SelectionRange")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)

    selectionInfo = BMessage()
    rez = reply.FindMessage("result", selectionInfo)
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *SelectionRange* FAILED! Error: {error}")
        return False

    if rez == 0:
        start_line = selectionInfo.GetInt32("start_line", -1)
        start_column = selectionInfo.GetInt32("start_column", -1)
        start_offset = selectionInfo.GetInt32("start_offset", -1)
        end_line = selectionInfo.GetInt32("end_line", -1)
        end_column = selectionInfo.GetInt32("end_column", -1)
        end_offset = selectionInfo.GetInt32("end_offset", -1)

        # Verify that we got valid values (all should be >= 0)
        if (start_line > 0 and start_column > 0 and start_offset >= 0 and
            end_line > 0 and end_column > 0 and end_offset >= 0 and
            start_offset == testStart and end_offset == (testStart + testLength)):
            print(f"test *SelectionRange* PASSED! Start: L{start_line}:C{start_column}({start_offset}), End: L{end_line}:C{end_column}({end_offset})")
            return True
        else:
            print(f"test *SelectionRange* FAILED! Invalid values - Start: L{start_line}:C{start_column}({start_offset}), End: L{end_line}:C{end_column}({end_offset})")
            print(f"  Expected offsets: start={testStart}, end={testStart + testLength}")
            return False
    else:
        print(f"test *SelectionRange* FAILED! FindMessage returned: {rez}")
        return False


def VisibleLines():
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("VisibleLines")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)

    visibleInfo = BMessage()
    rez = reply.FindMessage("result", visibleInfo)
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *VisibleLines* FAILED! Error: {error}")
        return False

    if rez == 0:
        first_line = visibleInfo.GetInt32("first_line", -1)
        last_line = visibleInfo.GetInt32("last_line", -1)

        # Verify that we got valid values
        if first_line > 0 and last_line >= first_line:
            visible_count = last_line - first_line + 1
            print(f"test *VisibleLines* PASSED! Visible range: L{first_line} to L{last_line} ({visible_count} lines)")
            return True
        else:
            print(f"test *VisibleLines* FAILED! Invalid values - first_line: {first_line}, last_line: {last_line}")
            return False
    else:
        print(f"test *VisibleLines* FAILED! FindMessage returned: {rez}")
        return False


def ScrollPosition():
    # First, get current scroll position
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("ScrollPosition")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)

    scrollInfo = BMessage()
    rez = reply.FindMessage("result", scrollInfo)
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *ScrollPosition (GET)* FAILED! Error: {error}")
        return False

    if rez != 0:
        print(f"test *ScrollPosition (GET)* FAILED! FindMessage returned: {rez}")
        return False

    x_offset = scrollInfo.GetInt32("x_offset", -1)
    first_visible_line = scrollInfo.GetInt32("first_visible_line", -1)

    if x_offset < 0 or first_visible_line <= 0:
        print(f"test *ScrollPosition (GET)* FAILED! Invalid values - x_offset: {x_offset}, first_visible_line: {first_visible_line}")
        return False

    print(f"test *ScrollPosition (GET)* PASSED! X offset: {x_offset}, First visible line: {first_visible_line}")

    # Test setting scroll position to line 10
    testScrollLine = 10
    setMessage = BMessage(B_SET_PROPERTY)
    setMessage.AddSpecifier("ScrollPosition")
    setMessage.AddSpecifier("SelectedEditor")
    setMessage.AddInt32("data", testScrollLine)
    setReply = BMessage()
    genio.SendMessage(setMessage, setReply)
    setError = setReply.GetInt32("error", 0)

    if setError != 0:
        print(f"test *ScrollPosition (SET)* FAILED! Error setting scroll: {setError}")
        return False

    # Wait a moment for the scroll to take effect
    time.sleep(0.1)

    # Verify the scroll position changed
    verifyMessage = BMessage(B_GET_PROPERTY)
    verifyMessage.AddSpecifier("ScrollPosition")
    verifyMessage.AddSpecifier("SelectedEditor")
    verifyReply = BMessage()
    genio.SendMessage(verifyMessage, verifyReply)

    verifyScrollInfo = BMessage()
    verifyRez = verifyReply.FindMessage("result", verifyScrollInfo)
    verifyError = verifyReply.GetInt32("error", 0)

    if verifyError != 0:
        print(f"test *ScrollPosition (SET)* FAILED! Verification error: {verifyError}")
        return False

    if verifyRez != 0:
        print(f"test *ScrollPosition (SET)* FAILED! Could not verify scroll position")
        return False

    new_first_visible = verifyScrollInfo.GetInt32("first_visible_line", -1)
    if new_first_visible == testScrollLine:
        print(f"test *ScrollPosition (SET)* PASSED! Successfully scrolled to line {new_first_visible}")
        return True
    else:
        print(f"test *ScrollPosition (SET)* FAILED! Expected line {testScrollLine}, got {new_first_visible}")
        return False


def Modified():
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Modified")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)

    modifiedInfo = BMessage()
    rez = reply.FindMessage("result", modifiedInfo)
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *Modified* FAILED! Error: {error}")
        return False

    if rez == 0:
        modified = modifiedInfo.GetBool("modified", False)
        can_undo = modifiedInfo.GetBool("can_undo", False)
        can_redo = modifiedInfo.GetBool("can_redo", False)

        # Since we've made changes in previous tests, the document should be modified
        # and should have undo capability
        if modified and can_undo:
            print(f"test *Modified* PASSED! Modified: {modified}, Can undo: {can_undo}, Can redo: {can_redo}")
            return True
        else:
            print(f"test *Modified* FAILED! Expected modified=True and can_undo=True, got modified={modified}, can_undo={can_undo}, can_redo={can_redo}")
            return False
    else:
        print(f"test *Modified* FAILED! FindMessage returned: {rez}")
        return False


def DocumentInfo():
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("DocumentInfo")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)

    docInfo = BMessage()
    rez = reply.FindMessage("result", docInfo)
    error = reply.GetInt32("error", 0)

    if error != 0:
        print(f"test *DocumentInfo* FAILED! Error: {error}")
        return False

    if rez == 0:
        length = docInfo.GetInt32("length", -1)
        line_count = docInfo.GetInt32("line_count", -1)
        text_length = docInfo.GetInt32("text_length", -1)

        # Verify that we got valid values (all should be > 0 for a non-empty document)
        if length > 0 and line_count > 0 and text_length > 0:
            print(f"test *DocumentInfo* PASSED! Length: {length} chars, Line count: {line_count}, Text length: {text_length}")
            return True
        else:
            print(f"test *DocumentInfo* FAILED! Invalid values - Length: {length}, Line count: {line_count}, Text length: {text_length}")
            return False
    else:
        print(f"test *DocumentInfo* FAILED! FindMessage returned: {rez}")
        return False


def main():
    print("=" * 70)
    print("GENIO SCRIPTING TEST SUITE")
    print("Test file: Alice's Adventures in Wonderland (Chapter I - excerpt)")
    print("=" * 70)
    print()

    # Track test results
    test_results = {}

    # Open the test file
    OpenEditor()
    time.sleep(1)

    # Run all tests and track results
    print("\n--- Running Tests ---\n")

    test_results["Selection"] = Selection()
    test_results["Line"] = Line()
    test_results["Undo (Line)"] = Undo()
    test_results["Insert"] = Insert()
    test_results["Undo (Insert)"] = Undo()
    test_results["Append"] = Append()
    test_results["Undo (Append)"] = Undo()
    test_results["Ref"] = Ref()
    test_results["CaretPosition"] = CaretPosition()
    test_results["SelectionRange"] = SelectionRange()
    test_results["VisibleLines"] = VisibleLines()
    test_results["ScrollPosition"] = ScrollPosition()
    test_results["DocumentInfo"] = DocumentInfo()

    # Make a modification to test the Modified property
    SetSelection(0, 0)
    message = BMessage(B_SET_PROPERTY)
    message.AddSpecifier("Text", 0)
    message.AddSpecifier("SelectedEditor")
    message.AddString("data", "TEST ")
    genio.SendMessage(message, None)
    time.sleep(0.1)  # Give the editor time to register the modification

    test_results["Modified"] = Modified()

    # Undo the test modification to clean up
    Undo()

    # Print summary
    print("\n" + "=" * 70)
    print("TEST SUMMARY")
    print("=" * 70)

    passed = sum(1 for result in test_results.values() if result)
    failed = sum(1 for result in test_results.values() if not result)
    total = len(test_results)

    for test_name, result in test_results.items():
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{status:8} | {test_name}")

    print("-" * 70)
    print(f"Total: {total} tests | Passed: {passed} | Failed: {failed}")
    print(f"Success rate: {(passed/total)*100:.1f}%")
    print("=" * 70)

    # Exit with appropriate code
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    exit(main())