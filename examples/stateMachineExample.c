/*
 * Copyright (c) 2013 Andreas Misje
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include "stateMachine.h"
#include <string.h>
/* This simple example checks keyboad input against the two allowed strings
 * "ha\n" and "hi\n". If an unrecognised character is read, a group state will
 * handle this by printing a message and returning to the idle state. If the
 * character '!' is encountered, a "reset" message is printed, and the group
 * state's entry state will be entered (the idle state).
 *
 *                   print 'reset'
 *       o      +---------------------+
 *       |      |                     | '!'
 *       |      v     group state     |
 * +-----v-----------------------------------+----+
 * |  +------+  'h'  +---+  'a'  +---+  '\n'      |
 * +->| idle | ----> | h | ----> | a | ---------+ |
 * |  +------+       +---+\      +---+          | |
 * |   ^ ^ ^               \'i'  +---+  '\n'    | |
 * |   | | |                \--> | i | ------+  | |
 * |   | | |                     +---+       |  | |
 * +---|-|-|----------------+----------------|--|-+
 *     | | |                |                |  |
 *     | | |                | '[^hai!\n]'    |  |
 *     | | | print unrecog. |                |  |
 *     | | +----------------+   print 'hi'   |  |
 *     | +-----------------------------------+  |
 *     |               print 'ha'               |
 *     +----------------------------------------+
 */
// TODO: 状态作为条件 - 可以
// TODO: eventType 更改条件 - 直接不回执行transions
// TODO: 父状态的eventType是否需要判断 - 需要判断
// TODO: 父状态如果都没有满足- 不执行任务转换
// TODO: eventType 可否为NULL - 不可以
// TODO: 多层级时,使用guard条件来做错误处理 - 可以
// TODO: 多层级的时候,顶层响应还是底层响应 - 底层响应
// TODO: transions 只能执行一个,还是满足条件都执行- 只执行一个,就进行状态切换了

/* Types of events */
enum eventType
{
    Event_keyboard,
    Event_test,
};

/* Compare keyboard character from transition's condition variable against
 * data in event. */
static bool compareKeyboardChar(void *ch, struct event *event);
static bool compareKeyboardCharWithState(void *ch, struct event *event);

static void printRecognisedChar(void *stateData, struct event *event);
static bool printUnrecognisedChar(void *oldStateData, struct event *event, void *newStateData);
static bool printReset(void *oldStateData, struct event *event, void *newStateData);
static bool printHiMsg(void *oldStateData, struct event *event, void *newStateData);
static bool printHaMsg(void *oldStateData, struct event *event, void *newStateData);
static bool printUnrecognisedCharGrandparent(void *oldStateData, struct event *event, void *newStateData);
static bool printUnrecognisedCharSun(void *oldStateData, struct event *event, void *newStateData);

static bool action_test1(void *oldStateData, struct event *event, void *newStateData);
static bool action_test2(void *oldStateData, struct event *event, void *newStateData);
static bool action_test3(void *oldStateData, struct event *event, void *newStateData);

static void printErrMsg(void *stateData, struct event *event);
static void printEnterMsg(void *stateData, struct event *event);
static void printExitMsg(void *stateData, struct event *event);

/* Forward declaration of states so that they can be defined in an logical
 * order: */
static struct state idleState, hState, iState, aState, errorState;
static struct state idleState1, eState, fState, gState;
static struct state checkCharsGroupState, testSunState;
static struct state testGrandparentState;

/* All the following states (apart from the error state) are children of this
 * group state. This way, any unrecognised character will be handled by this
 * state's transition, eliminating the need for adding the same transition to
 * all the children states. */
static struct state testGrandparentState = {
    .parentState = NULL,
    /* The entry state is defined in order to demontrate that the 'reset'
     * transtition, going to this group state, will be 'redirected' to the
     * 'idle' state (the transition could of course go directly to the 'idle'
     * state): */
    .entryState = &checkCharsGroupState,
    .transitions = (struct transition[]){
        {
            Event_keyboard,
            (void *)(intptr_t)'@',
            &compareKeyboardChar,
            &printReset,
            &testSunState,
        },
        {
            Event_keyboard,
            (void *)(intptr_t)'l',
            &compareKeyboardChar,
            &printUnrecognisedCharGrandparent,
            &idleState,
        },
        {
            Event_keyboard,
            NULL,
            NULL,
            &printUnrecognisedCharGrandparent,
            &idleState,
        },
        //   {
        //       NULL,
        //       NULL,
        //       NULL,
        //       &action_test3,
        //       &idleState,
        //   },

    },
    .numTransitions = 3,
    .data = "Grandparent group",
    .entryAction = &printEnterMsg,
    .exitAction = &printExitMsg,
};
static struct state checkCharsGroupState = {
    .parentState = &testGrandparentState,
    /* The entry state is defined in order to demontrate that the 'reset'
     * transtition, going to this group state, will be 'redirected' to the
     * 'idle' state (the transition could of course go directly to the 'idle'
     * state): */
    .entryState = &idleState,
    .transitions = (struct transition[]){
        {
            Event_keyboard,
            (void *)(intptr_t)'!',
            &compareKeyboardChar,
            &printReset,
            &idleState,
        },
        {
            Event_keyboard,
            (void *)(intptr_t)'l',
            &compareKeyboardChar,
            &printUnrecognisedChar,
            &idleState,
        },
    },
    .numTransitions = 2,
    .data = "group",
    .entryAction = &printEnterMsg,
    .exitAction = &printExitMsg,
};
static struct state testSunState = {
    .parentState = &testGrandparentState,
    /* The entry state is defined in order to demontrate that the 'reset'
     * transtition, going to this group state, will be 'redirected' to the
     * 'idle' state (the transition could of course go directly to the 'idle'
     * state): */
    .entryState = &idleState1,
    .transitions = (struct transition[]){
        {
            Event_keyboard,
            (void *)(intptr_t)'@',
            &compareKeyboardChar,
            &printReset,
            &idleState1,
        },
        {
            Event_keyboard,
            (void *)(intptr_t)'k',
            &compareKeyboardChar,
            &printUnrecognisedCharSun,
            &idleState1,
        },
    },
    .numTransitions = 2,
    .data = "Sun group",
    .entryAction = &printEnterMsg,
    .exitAction = &printExitMsg,
};

static struct state idleState = {
    .parentState = &checkCharsGroupState,
    .entryState = NULL,
    .transitions = (struct transition[]){
        {Event_test, (void *)&idleState, &compareKeyboardCharWithState, action_test1, &hState},
        {Event_keyboard, (void *)(intptr_t)'h', &compareKeyboardChar, action_test2, &hState},

    },
    .numTransitions = 2,
    .data = "idle",
    .entryAction = &printEnterMsg,
    .exitAction = &printExitMsg,
};

static struct state hState = {
    .parentState = &checkCharsGroupState,
    .entryState = NULL,
    .transitions = (struct transition[]){
        {Event_keyboard, (void *)(intptr_t)'a', &compareKeyboardChar, NULL,
         &aState},
        {Event_keyboard, (void *)(intptr_t)'i', &compareKeyboardChar, NULL,
         &iState},
    },
    .numTransitions = 2,
    .data = "H",
    .entryAction = &printRecognisedChar,
    .exitAction = &printExitMsg,
};

static struct state iState = {
    .parentState = &checkCharsGroupState,
    .entryState = NULL,
    .transitions = (struct transition[]){
        {Event_keyboard, (void *)(intptr_t)'\n', &compareKeyboardChar, &printHiMsg, &idleState}},
    .numTransitions = 1,
    .data = "I",
    .entryAction = &printRecognisedChar,
    .exitAction = &printExitMsg,
};
static struct state aState = {
    .parentState = &checkCharsGroupState,
    .entryState = NULL,
    .transitions = (struct transition[]){
        {Event_keyboard, (void *)(intptr_t)'\n', &compareKeyboardChar, &printHaMsg, &idleState}},
    .numTransitions = 1,
    .data = "A",
    .entryAction = NULL,
    .exitAction = &printExitMsg};

static struct state idleState1 = {
    .parentState = &testSunState,
    .entryState = NULL,
    .transitions = (struct transition[]){
        {Event_keyboard, (void *)(intptr_t)'e', &compareKeyboardChar, NULL, &eState},
    },
    .numTransitions = 1,
    .data = "idle111111",
    .entryAction = &printEnterMsg,
    .exitAction = &printExitMsg,
};

static struct state eState = {
    .parentState = &testSunState,
    .entryState = NULL,
    .transitions = (struct transition[]){
        {Event_keyboard, (void *)(intptr_t)'f', &compareKeyboardChar, NULL, &fState},
        {Event_keyboard, (void *)(intptr_t)'g', &compareKeyboardChar, NULL, &gState},
    },
    .numTransitions = 2,
    .data = "E",
    .entryAction = &printRecognisedChar,
    .exitAction = &printExitMsg,
};

static struct state fState = {
    .parentState = &testSunState,
    .entryState = NULL,
    .transitions = (struct transition[]){
        {Event_keyboard, (void *)(intptr_t)'\n', &compareKeyboardChar, &printHiMsg, &idleState1}},
    .numTransitions = 1,
    .data = "F",
    .entryAction = &printRecognisedChar,
    .exitAction = &printExitMsg,
};

static struct state gState = {
    .parentState = &testSunState,
    .entryState = NULL,
    .transitions = (struct transition[]){
        {Event_keyboard, (void *)(intptr_t)'\n', &compareKeyboardChar, &printHaMsg, &idleState1}},
    .numTransitions = 1,
    .data = "G",
    .entryAction = &printRecognisedChar,
    .exitAction = &printExitMsg};

static struct state errorState = {
    .entryAction = &printErrMsg};

struct stateMachine m;

int main()
{
    stateM_init(&m, &idleState, &errorState);

    int ch;
    while ((ch = getc(stdin)) != EOF)
        if (ch != 'h' && ch != 'i' && ch != 'a' && ch != '\n' && ch != '!')
        {
            stateM_handleEvent(&m, &(struct event){Event_test, (void *)(intptr_t)ch});
        }
        else
        {
            stateM_handleEvent(&m, &(struct event){Event_keyboard, (void *)(intptr_t)ch});
        }

    return 0;
}

static bool compareKeyboardChar(void *ch, struct event *event)
{
    // if (event->type != Event_keyboard)
    //    return false;

    return (intptr_t)ch == (intptr_t)event->data;
}

static bool compareKeyboardCharWithState(void *ch, struct event *event)
{
    // if (event->type != Event_keyboard)
    //    return false;
    printf("compareKeyboardCharWithState\n");
    // return false;
    return (struct state *)ch == (struct state *)stateM_currentState(&m);
}

static void printRecognisedChar(void *stateData, struct event *event)
{
    printEnterMsg(stateData, event);
    printf("parsed: %c\n", (char)(intptr_t)event->data);
}

static bool printUnrecognisedChar(void *oldStateData, struct event *event,
                                  void *newStateData)
{
    printf("old state: %s\n", (char *)oldStateData);
    printf("new state: %s\n", (char *)newStateData);
    printf("unrecognised character: %c\n",
           (char)(intptr_t)event->data);
    return true;
}

static bool printReset(void *oldStateData, struct event *event,
                       void *newStateData)
{
    printf("old state: %s\n", (char *)oldStateData);
    printf("new state: %s\n", (char *)newStateData);
    puts("Resetting");
    return true;
}

static bool printHiMsg(void *oldStateData, struct event *event,
                       void *newStateData)
{
    puts("Hi!");

    return false;
}

static bool printHaMsg(void *oldStateData, struct event *event,
                       void *newStateData)
{
    puts("Ha-ha");
    return true;
}

static void printErrMsg(void *stateData, struct event *event)
{
    puts("ENTERED ERROR STATE!");
}

static void printEnterMsg(void *stateData, struct event *event)
{
    printf("Entering %s state\n", (char *)stateData);
}

static void printExitMsg(void *stateData, struct event *event)
{
    printf("Exiting %s state\n", (char *)stateData);
}

static bool printUnrecognisedCharGrandparent(void *oldStateData, struct event *event, void *newStateData)
{
    printf("old state: %s\n", (char *)oldStateData);
    printf("new state: %s\n", (char *)newStateData);
    printf("Grandparent unrecognised character: %c\n",
           (char)(intptr_t)event->data);
    return true;
}
static bool printUnrecognisedCharSun(void *oldStateData, struct event *event, void *newStateData)
{
    printf("old state: %s\n", (char *)oldStateData);
    printf("new state: %s\n", (char *)newStateData);
    printf("Sun unrecognised character: %c\n", (char)(intptr_t)event->data);
    return true;
}

static bool action_test1(void *oldStateData, struct event *event, void *newStateData)
{
    printf("test111111\n");
    return true;
}
static bool action_test2(void *oldStateData, struct event *event, void *newStateData)
{
    printf("test222222\n");
    return true;
}

static bool action_test3(void *oldStateData, struct event *event, void *newStateData)
{
    printf("test33333333\n");
    return true;
}
