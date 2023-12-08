/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file contains a standard set of commands that are used by the
 * esp8266 web interface.
 * You can add your own commands if needed
 */
#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/memorymap.h>
#include "hwdefs.h"
#include "terminal.h"
#include "params.h"
#include "my_string.h"
#include "my_fp.h"
#include "printf.h"
#include "param_save.h"
#include "errormessage.h"
#include "terminalcommands.h"

static void LoadDefaults(Terminal* term, char *arg);
static void Help(Terminal* term, char *arg);
static void PrintSerial(Terminal* term, char *arg);
static void PrintErrors(Terminal* term, char *arg);

extern "C" const TERM_CMD termCmds[] =
{
  { "set", TerminalCommands::ParamSet },
  { "get", TerminalCommands::ParamGet },
  { "flag", TerminalCommands::ParamFlag },
  { "stream", TerminalCommands::ParamStream },
  { "json", TerminalCommands::PrintParamsJson },
  { "can", TerminalCommands::MapCan },
  { "save", TerminalCommands::SaveParameters },
  { "load", TerminalCommands::LoadParameters },
  { "reset", TerminalCommands::Reset },
  { "defaults", LoadDefaults },
  { "help", Help },
  { "serial", PrintSerial },
  { "errors", PrintErrors },
  { NULL, NULL }
};

static void LoadDefaults(Terminal* term, char *arg)
{
   arg = arg;
   Param::LoadDefaults();
   fprintf(term, "Defaults loaded\r\n");
}

static void PrintErrors(Terminal* term, char *arg)
{
   arg = arg;
   term = term;
   ErrorMessage::PrintAllErrors();
}

static void PrintSerial(Terminal* term, char *arg)
{
   arg = arg;
   fprintf(term, "%08X:%08X:%08X\r\n", DESIG_UNIQUE_ID2, DESIG_UNIQUE_ID1, DESIG_UNIQUE_ID0);
}

static void Help(Terminal* term, char *arg)
{
   //If you want you could print some instructions here
   //But since the terminal is mostly used by the web interface
   //it makes limited sense.
   arg = arg;
   term = term;
}
