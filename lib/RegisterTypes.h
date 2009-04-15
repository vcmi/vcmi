#pragma  once
//templates for registering object types

//first set of types - derivatives of CGObjectInstance
template<typename Serializer> DLL_EXPORT
void registerTypes1(Serializer &s);


//second set of types - derivatives of CPack for client (network VCMI packages)
template<typename Serializer> DLL_EXPORT
void registerTypes2(Serializer &s);

template<typename Serializer> DLL_EXPORT
void registerTypes3(Serializer &s);

//register all
template<typename Serializer> DLL_EXPORT
void registerTypes(Serializer &s);

/*
 * RegisterTypes.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */