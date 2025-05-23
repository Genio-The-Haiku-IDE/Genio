PlatHaiku.o: PlatHaiku.cxx ../include/ScintillaTypes.h \
 ../include/ScintillaMessages.h ../src/Debugging.h ../src/Geometry.h \
 ../src/Platform.h ../include/Scintilla.h ../include/Sci_Position.h \
 ../src/XPM.h ../src/UniConversion.h
ScintillaHaiku.o: ScintillaHaiku.cxx ../include/ScintillaTypes.h \
 ../include/ScintillaMessages.h ../include/ScintillaStructures.h \
 ../include/ILoader.h ../include/Sci_Position.h ../include/ILexer.h \
 ../src/Debugging.h ../src/Geometry.h ../src/Platform.h \
 ../include/Scintilla.h ScintillaView.h ../src/CharacterCategoryMap.h \
 ../src/Position.h ../src/UniqueString.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/ContractionState.h \
 ../src/CellBuffer.h ../src/CallTip.h ../src/KeyMap.h ../src/Indicator.h \
 ../src/XPM.h ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/Selection.h ../src/PositionCache.h \
 ../src/EditModel.h ../src/MarginView.h ../src/EditView.h ../src/Editor.h \
 ../src/AutoComplete.h ../src/ScintillaBase.h
AutoComplete.o: ../src/AutoComplete.cxx ../include/ScintillaTypes.h \
 ../include/ScintillaMessages.h ../src/Debugging.h ../src/Geometry.h \
 ../src/Platform.h ../src/CharacterType.h ../src/Position.h \
 ../src/AutoComplete.h
CallTip.o: ../src/CallTip.cxx ../include/ScintillaTypes.h \
 ../include/ScintillaMessages.h ../src/Debugging.h ../src/Geometry.h \
 ../src/Platform.h ../src/Position.h ../src/CallTip.h
CaseConvert.o: ../src/CaseConvert.cxx ../src/CaseConvert.h \
 ../src/UniConversion.h
CaseFolder.o: ../src/CaseFolder.cxx ../src/CharacterType.h \
 ../src/CaseFolder.h ../src/CaseConvert.h
CellBuffer.o: ../src/CellBuffer.cxx ../include/ScintillaTypes.h \
 ../src/Debugging.h ../src/Position.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/SparseVector.h \
 ../src/ChangeHistory.h ../src/CellBuffer.h ../src/UndoHistory.h \
 ../src/UniConversion.h
ChangeHistory.o: ../src/ChangeHistory.cxx ../include/ScintillaTypes.h \
 ../src/Debugging.h ../src/Position.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/SparseVector.h \
 ../src/ChangeHistory.h
CharacterCategoryMap.o: ../src/CharacterCategoryMap.cxx \
 ../src/CharacterCategoryMap.h
CharacterType.o: ../src/CharacterType.cxx ../src/CharacterType.h
CharClassify.o: ../src/CharClassify.cxx ../src/CharacterType.h \
 ../src/CharClassify.h
ContractionState.o: ../src/ContractionState.cxx ../src/Debugging.h \
 ../src/Position.h ../src/UniqueString.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/SparseVector.h \
 ../src/ContractionState.h
DBCS.o: ../src/DBCS.cxx ../src/DBCS.h
Decoration.o: ../src/Decoration.cxx ../include/ScintillaTypes.h \
 ../src/Debugging.h ../src/Position.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/Decoration.h
Document.o: ../src/Document.cxx ../include/ScintillaTypes.h \
 ../include/ILoader.h ../include/Sci_Position.h ../include/ILexer.h \
 ../src/Debugging.h ../src/CharacterType.h ../src/CharacterCategoryMap.h \
 ../src/Position.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/CellBuffer.h ../src/PerLine.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/RESearch.h ../src/UniConversion.h \
 ../src/ElapsedPeriod.h
EditModel.o: ../src/EditModel.cxx ../include/ScintillaTypes.h \
 ../include/ILoader.h ../include/Sci_Position.h ../include/ILexer.h \
 ../src/Debugging.h ../src/Geometry.h ../src/Platform.h \
 ../src/CharacterCategoryMap.h ../src/Position.h ../src/UniqueString.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h ../src/CellBuffer.h ../src/Indicator.h \
 ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/UniConversion.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h
Editor.o: ../src/Editor.cxx ../include/ScintillaTypes.h \
 ../include/ScintillaMessages.h ../include/ScintillaStructures.h \
 ../include/ILoader.h ../include/Sci_Position.h ../include/ILexer.h \
 ../src/Debugging.h ../src/Geometry.h ../src/Platform.h \
 ../src/CharacterType.h ../src/CharacterCategoryMap.h ../src/Position.h \
 ../src/UniqueString.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/ContractionState.h ../src/CellBuffer.h \
 ../src/PerLine.h ../src/KeyMap.h ../src/Indicator.h ../src/LineMarker.h \
 ../src/Style.h ../src/ViewStyle.h ../src/CharClassify.h \
 ../src/Decoration.h ../src/CaseFolder.h ../src/Document.h \
 ../src/UniConversion.h ../src/DBCS.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h ../src/MarginView.h \
 ../src/EditView.h ../src/Editor.h ../src/ElapsedPeriod.h
EditView.o: ../src/EditView.cxx ../include/ScintillaTypes.h \
 ../include/ScintillaMessages.h ../include/ScintillaStructures.h \
 ../include/ILoader.h ../include/Sci_Position.h ../include/ILexer.h \
 ../src/Debugging.h ../src/Geometry.h ../src/Platform.h \
 ../src/CharacterType.h ../src/CharacterCategoryMap.h ../src/Position.h \
 ../src/UniqueString.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/ContractionState.h ../src/CellBuffer.h \
 ../src/PerLine.h ../src/KeyMap.h ../src/Indicator.h ../src/LineMarker.h \
 ../src/Style.h ../src/ViewStyle.h ../src/CharClassify.h \
 ../src/Decoration.h ../src/CaseFolder.h ../src/Document.h \
 ../src/UniConversion.h ../src/Selection.h ../src/PositionCache.h \
 ../src/EditModel.h ../src/MarginView.h ../src/EditView.h \
 ../src/ElapsedPeriod.h
Geometry.o: ../src/Geometry.cxx ../src/Geometry.h
Indicator.o: ../src/Indicator.cxx ../include/ScintillaTypes.h \
 ../src/Debugging.h ../src/Geometry.h ../src/Platform.h \
 ../src/Indicator.h ../src/XPM.h
KeyMap.o: ../src/KeyMap.cxx ../include/ScintillaTypes.h \
 ../include/ScintillaMessages.h ../src/Debugging.h ../src/Geometry.h \
 ../src/Platform.h ../src/KeyMap.h
LineMarker.o: ../src/LineMarker.cxx ../include/ScintillaTypes.h \
 ../src/Debugging.h ../src/Geometry.h ../src/Platform.h ../src/XPM.h \
 ../src/LineMarker.h ../src/UniConversion.h
MarginView.o: ../src/MarginView.cxx ../include/ScintillaTypes.h \
 ../include/ScintillaMessages.h ../include/ScintillaStructures.h \
 ../include/ILoader.h ../include/Sci_Position.h ../include/ILexer.h \
 ../src/Debugging.h ../src/Geometry.h ../src/Platform.h \
 ../src/CharacterCategoryMap.h ../src/Position.h ../src/UniqueString.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h ../src/CellBuffer.h ../src/KeyMap.h \
 ../src/Indicator.h ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/UniConversion.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h ../src/MarginView.h \
 ../src/EditView.h
PerLine.o: ../src/PerLine.cxx ../include/ScintillaTypes.h \
 ../src/Debugging.h ../src/Geometry.h ../src/Platform.h ../src/Position.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/CellBuffer.h \
 ../src/PerLine.h
PositionCache.o: ../src/PositionCache.cxx ../include/ScintillaTypes.h \
 ../include/ScintillaMessages.h ../include/ILoader.h \
 ../include/Sci_Position.h ../include/ILexer.h ../src/Debugging.h \
 ../src/Geometry.h ../src/Platform.h ../src/CharacterType.h \
 ../src/CharacterCategoryMap.h ../src/Position.h ../src/UniqueString.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h ../src/CellBuffer.h ../src/KeyMap.h \
 ../src/Indicator.h ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/UniConversion.h ../src/DBCS.h \
 ../src/Selection.h ../src/PositionCache.h
RESearch.o: ../src/RESearch.cxx ../src/Position.h ../src/CharClassify.h \
 ../src/RESearch.h
RunStyles.o: ../src/RunStyles.cxx ../src/Debugging.h ../src/Position.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h
ScintillaBase.o: ../src/ScintillaBase.cxx ../include/ScintillaTypes.h \
 ../include/ScintillaMessages.h ../include/ScintillaStructures.h \
 ../include/ILoader.h ../include/Sci_Position.h ../include/ILexer.h \
 ../src/Debugging.h ../src/Geometry.h ../src/Platform.h \
 ../src/CharacterCategoryMap.h ../src/Position.h ../src/UniqueString.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/ContractionState.h ../src/CellBuffer.h ../src/CallTip.h \
 ../src/KeyMap.h ../src/Indicator.h ../src/LineMarker.h ../src/Style.h \
 ../src/ViewStyle.h ../src/CharClassify.h ../src/Decoration.h \
 ../src/CaseFolder.h ../src/Document.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h ../src/MarginView.h \
 ../src/EditView.h ../src/Editor.h ../src/AutoComplete.h \
 ../src/ScintillaBase.h
Selection.o: ../src/Selection.cxx ../src/Debugging.h ../src/Position.h \
 ../src/Selection.h
Style.o: ../src/Style.cxx ../include/ScintillaTypes.h ../src/Debugging.h \
 ../src/Geometry.h ../src/Platform.h ../src/Style.h
UndoHistory.o: ../src/UndoHistory.cxx ../include/ScintillaTypes.h \
 ../src/Debugging.h ../src/Position.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/SparseVector.h \
 ../src/ChangeHistory.h ../src/CellBuffer.h ../src/UndoHistory.h
UniConversion.o: ../src/UniConversion.cxx ../src/UniConversion.h
UniqueString.o: ../src/UniqueString.cxx ../src/UniqueString.h
ViewStyle.o: ../src/ViewStyle.cxx ../include/ScintillaTypes.h \
 ../src/Debugging.h ../src/Geometry.h ../src/Platform.h ../src/Position.h \
 ../src/UniqueString.h ../src/Indicator.h ../src/XPM.h \
 ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h
XPM.o: ../src/XPM.cxx ../include/ScintillaTypes.h ../src/Debugging.h \
 ../src/Geometry.h ../src/Platform.h ../src/XPM.h
