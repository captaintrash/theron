// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_DIRECTORY_GLOBALDIRECTORY_H
#define THERON_DETAIL_DIRECTORY_GLOBALDIRECTORY_H


#include <Theron/Detail/Directory/Directory.h>


namespace Theron
{
namespace Detail
{


// Process-wide directory in which frameworks and receivers are registered.
extern Directory sGlobalDirectory;


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_DIRECTORY_GLOBALDIRECTORY_H

