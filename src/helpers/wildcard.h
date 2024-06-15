// Wildest card is a tiny single-header wildcard matching library
// Written by:   Roman Shchekin aka QtRoS
// Licence:      MIT https://choosealicense.com/licenses/mit/
// Project home: https://github.com/QtRoS/wildest-card
// Version:      v1.2.1

#ifndef WILDCARD_H
#define WILDCARD_H

#include <memory.h>
#include <stdbool.h>

// High level defines, can be castomized to wchar_t and wcslen for example.
#define STR_TYPE char
#define STR_LEN strlen
#define STAR_CHARACTER '*'
#define QUESTION_CHARACTER '?'

// Low level defines, used in compact NFA implementation.
#define BITS_IN_CHAR (sizeof(char) * 8)
#define BIT_IN_CHAR_LOG2 3
#define MAX_WILDCARD_LEN 255  // Only 2^n-1 values are acceptable, e.g. 7, 31, 1023 

// Very fast and compact bitwise NFA implementation.
#define addState(_bitArray, _pos) \
    { \
        char *bitArray = _bitArray; \
        int pos = _pos; \
        (bitArray)[(pos) / BITS_IN_CHAR] = (bitArray)[(pos) / BITS_IN_CHAR] | ( 1 << ((pos) % BITS_IN_CHAR )); \
    }

#define checkState(bitArray,pos) (bitArray[pos / BITS_IN_CHAR] & ( 1 << (pos % BITS_IN_CHAR )))

#define resetStates(_pStates, _size) memset(_pStates, 0, _size)

// Wildcard implementation itself.
inline bool wildcard(STR_TYPE const * pattern, STR_TYPE const* input)
{
    if (!pattern || !input)
        return false;

    int i, j;
    int inputLength = STR_LEN(input);
    int patternLength = STR_LEN(pattern);
    patternLength = patternLength > MAX_WILDCARD_LEN ? MAX_WILDCARD_LEN : patternLength;

    int state = 0;
    int nfaStateCount = patternLength + 1; // Extra 1 for accepting state of NFA.
    const int sizeInBytes = (nfaStateCount >> BIT_IN_CHAR_LOG2) + (nfaStateCount & 0x7 ? 1 : 0);

    // Only sizeInBytes bytes will be used for NFA.
    char nfaCurrStates[(MAX_WILDCARD_LEN + 1) / BITS_IN_CHAR] = {0};
    char nfaNextStates[(MAX_WILDCARD_LEN + 1) / BITS_IN_CHAR] = {0};

    char* pCurrStates = nfaCurrStates;
    char* pNextStates = nfaNextStates;
    char* pSwap;

    // NFA starts in zero state. Here and further: while corresponding pattern symbol
    // is equal STAR_CHARACTER, we do epsilon-transition to the next state. 
    addState(pCurrStates, state);
    while (state < patternLength && pattern[state] == STAR_CHARACTER)
        addState(pCurrStates, ++state);

    for (i = 0; i < inputLength; i++)
    {
        for (j = 0; j < patternLength; j++)
        {
            state = j;
            if (!checkState(pCurrStates, state))
                continue;

            STR_TYPE c = pattern[state];

            if (c == STAR_CHARACTER)
            {
                addState(pNextStates, state);
                while (state < patternLength && pattern[state] == STAR_CHARACTER)
                    addState(pNextStates, ++state);
            }
            else if (c == QUESTION_CHARACTER || input[i] == c)
            {
                addState(pNextStates, ++state);
                while (state < patternLength && pattern[state] == STAR_CHARACTER)
                    addState(pNextStates, ++state);
            }
        }

        pSwap = pCurrStates;
        pCurrStates = pNextStates;
        pNextStates = pSwap;

        resetStates(pNextStates, sizeInBytes);
    }

    bool result = checkState(pCurrStates, patternLength); // Check if NFA is in accepting state.
    return result;
}
#endif
