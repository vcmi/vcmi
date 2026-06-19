/*
 * DocExport.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "DocExport.h"

#include "DocRegistrar.h"
#include "FieldDocRegistrar.h"
#include "Registry.h"

#include <boost/filesystem/operations.hpp>

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

namespace
{

/// Splits a type string like "Unit[]" or "BattleHex?" into (atom, suffix). The atom is what
/// the link target lookup uses; the suffix is appended back to the displayed link text so
/// readers still see the decoration ("Unit[]"), but clicking jumps to "Unit.md".
struct TypeAtom
{
	std::string atom;
	std::string suffix;
};

TypeAtom splitTypeDecoration(const std::string & type)
{
	TypeAtom out{type, {}};
	while(!out.atom.empty() && (out.atom.back() == '?' || out.atom.back() == ']'))
	{
		if(out.atom.back() == '?')
		{
			out.suffix.insert(out.suffix.begin(), '?');
			out.atom.pop_back();
		}
		else
		{
			// "[]" — pop both
			if(out.atom.size() >= 2 && out.atom[out.atom.size() - 2] == '[')
			{
				out.suffix.insert(out.suffix.begin(), ']');
				out.suffix.insert(out.suffix.begin(), '[');
				out.atom.pop_back();
				out.atom.pop_back();
			}
			else
				break;
		}
	}
	return out;
}

/// Renders a type for an inline Markdown context. If the atom names a registered type, the
/// rendered form is a Markdown link to that type's file; otherwise it's plain inline code.
std::string renderType(const std::string & type, const std::set<std::string> & knownTypes)
{
	if(type.empty())
		return {};

	const auto split = splitTypeDecoration(type);
	if(knownTypes.find(split.atom) != knownTypes.end())
		return "[`" + split.atom + split.suffix + "`](" + split.atom + ".md)";

	return "`" + type + "`";
}

/// Skips a leading "true if" / "returns" / "true when" so the param/return bullet description
/// reads naturally after the "— " separator. We keep the caller's description as-is — this
/// helper just trims leading whitespace.
std::string trimLeading(const std::string & s)
{
	std::size_t i = 0;
	while(i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n'))
		++i;
	return s.substr(i);
}

/// Emits one method/cfunction/function entry as an H3 block:
///   ### name
///   <description>
///    - param `name`: <type> — <description>
///    - returns <type> — <description>
void emitMethodEntry(std::ostream & out,
                     const DocRegistrar::Entry & entry,
                     const std::set<std::string> & knownTypes)
{
	out << "### " << entry.name << "\n\n";
	if(!entry.description.empty())
		out << entry.description << "\n\n";

	if(!entry.params.empty())
	{
		for(const auto & p : entry.params)
		{
			out << "- param `" << p.name << "`: " << renderType(p.type, knownTypes);
			if(!p.description.empty())
				out << " — " << trimLeading(p.description);
			out << "\n";
		}
		out << "\n";
	}

	if(!entry.ret.type.empty())
	{
		out << "- returns " << renderType(entry.ret.type, knownTypes);
		if(!entry.ret.description.empty())
			out << " — " << trimLeading(entry.ret.description);
		out << "\n\n";
	}
}

/// Emits one field entry as an H3 block:
///   ### name
///   <description>
///    - type: <type>
void emitFieldEntry(std::ostream & out,
                    const FieldDocRegistrar::Entry & entry,
                    const std::set<std::string> & knownTypes)
{
	out << "### " << entry.name << "\n\n";
	if(!entry.description.empty())
		out << entry.description << "\n\n";
	out << "- type: " << renderType(entry.type, knownTypes) << "\n\n";
}

/// Emits one "field that is itself an enum group" entry: paragraph + link to its dedicated file.
void emitEnumGroupLink(std::ostream & out, const FieldDocRegistrar::EnumGroupEntry & group)
{
	out << "### " << group.name << "\n\n";
	if(!group.description.empty())
		out << group.description << "\n\n";
	out << "- type: [`" << group.name << "`](" << group.name << ".md)\n\n";
}

/// Writes one Markdown file for a single class/serializable type.
void emitClassFile(const boost::filesystem::path & outDir,
                   const std::string & typeName,
                   std::string_view description,
                   const std::vector<DocRegistrar::Entry> & methods,
                   const std::vector<FieldDocRegistrar::Entry> & fields,
                   const std::vector<FieldDocRegistrar::EnumGroupEntry> & enumGroups,
                   const std::set<std::string> & knownTypes)
{
	std::ostringstream body;

	body << "# " << typeName << "\n\n";
	if(!description.empty())
		body << description << "\n\n";

	for(const auto & entry : methods)
		emitMethodEntry(body, entry, knownTypes);

	for(const auto & entry : fields)
		emitFieldEntry(body, entry, knownTypes);

	for(const auto & group : enumGroups)
		emitEnumGroupLink(body, group);

	if(methods.empty() && fields.empty() && enumGroups.empty())
		body << "_No bindings exposed._\n\n";

	std::string content = body.str();
	while(content.size() >= 2 && content[content.size() - 1] == '\n' && content[content.size() - 2] == '\n')
		content.pop_back();

	std::ofstream out((outDir / (typeName + ".md")).string());
	out << content;
}

/// Writes one Markdown file for a single enum group.
void emitEnumFile(const boost::filesystem::path & outDir,
                  const FieldDocRegistrar::EnumGroupEntry & group)
{
	std::ofstream out((outDir / (group.name + ".md")).string());

	out << "# " << group.name << "\n\n";
	if(!group.description.empty())
		out << group.description << "\n\n";

	out << "| Key | Value | Description |\n";
	out << "| --- | ----- | ----------- |\n";
	for(const auto & key : group.keys)
	{
		out << "| `" << key.key << "` | " << key.value << " | " << key.description << " |\n";
	}
}

/// Writes the API.md index that links to every class and enum page.
void emitIndex(const boost::filesystem::path & outDir,
               const std::vector<std::string> & classNames,
               const std::vector<std::string> & enumNames)
{
	std::ofstream out((outDir / "API.md").string());
	out << "# Lua scripting API reference\n\n"
	    << "Auto-generated from C++ host bindings via `vcmiserver --export-lua-docs`. Do not edit by hand.\n\n";

	if(!classNames.empty())
	{
		out << "## Classes\n\n";
		for(const auto & name : classNames)
			out << "- [" << name << "](" << name << ".md)\n";
		out << "\n";
	}

	if(!enumNames.empty())
	{
		out << "## Enums\n\n";
		for(const auto & name : enumNames)
			out << "- [" << name << "](" << name << ".md)\n";
	}
}

/// Wraps a description across multiple `---` lines so luals' hover popup renders
/// each line as a separate paragraph instead of one long row.
void emitLualsDescription(std::ofstream & out, std::string_view description)
{
	if(description.empty())
		return;

	std::size_t start = 0;
	while(start < description.size())
	{
		const auto end = description.find('\n', start);
		const auto len = (end == std::string_view::npos) ? description.size() - start : end - start;
		out << "---" << description.substr(start, len) << "\n";
		if(end == std::string_view::npos)
			break;
		start = end + 1;
	}
}

/// Emits a single function declaration as Lua Language Server annotations.
void emitLualsMethod(std::ofstream & out, const std::string & className, const DocRegistrar::Entry & entry)
{
	if(!entry.description.empty())
		emitLualsDescription(out, entry.description);

	for(const auto & p : entry.params)
	{
		out << "---@param " << p.name << ' ' << (p.type.empty() ? std::string("any") : p.type);
		if(!p.description.empty())
			out << " # " << p.description;
		out << "\n";
	}

	if(!entry.ret.type.empty())
	{
		out << "---@return " << entry.ret.type;
		if(!entry.ret.description.empty())
			out << " # " << entry.ret.description;
		out << "\n";
	}

	out << "function " << className << ":" << entry.name << "(";
	for(std::size_t i = 0; i < entry.params.size(); ++i)
	{
		if(i)
			out << ", ";
		out << entry.params[i].name;
	}
	out << ") end\n\n";
}

void emitLualsType(std::ofstream & out,
                   const std::string & typeName,
                   std::string_view description,
                   const std::vector<DocRegistrar::Entry> & methods,
                   const std::vector<FieldDocRegistrar::Entry> & fields,
                   const std::vector<FieldDocRegistrar::EnumGroupEntry> & enumGroups)
{
	emitLualsDescription(out, description);
	out << "---@class " << typeName << "\n";

	for(const auto & entry : fields)
	{
		out << "---@field " << entry.name << ' ' << entry.type;
		if(!entry.description.empty())
			out << " # " << entry.description;
		out << "\n";
	}

	// Enum groups appear on the parent class (e.g. `ENUM`) as fields whose value type is
	// the per-group `---@enum` declared below. LuaLS auto-completes `ENUM.HealLevel.heal`
	// and gives hover docs from this field's description.
	for(const auto & group : enumGroups)
	{
		out << "---@field " << group.name << ' ' << group.name;
		if(!group.description.empty())
			out << " # " << group.description;
		out << "\n";
	}

	out << "local " << typeName << " = {}\n\n";

	for(const auto & entry : methods)
		emitLualsMethod(out, typeName, entry);

	// Each enum group becomes a standalone `---@enum` declaration with the captured key→
	// integer pairs, one `---<description>` line per key.
	for(const auto & group : enumGroups)
	{
		emitLualsDescription(out, group.description);
		out << "---@enum " << group.name << "\n";
		out << "local " << group.name << " = {\n";
		for(const auto & key : group.keys)
		{
			if(!key.description.empty())
				out << "    ---" << key.description << "\n";
			out << "    " << key.key << " = " << key.value << ",\n";
		}
		out << "}\n\n";
	}
}

}

void exportLuaApiDocs(const boost::filesystem::path & outDir)
{
	boost::filesystem::create_directories(outDir);

	const auto & types = Registry::get()->getAllTypes();

	// First pass: collect every name that has its own dedicated file so renderType() can
	// turn occurrences of those names into links. Includes both top-level classes and the
	// enum groups that get extracted to their own pages.
	std::set<std::string> knownTypes;
	std::vector<std::string> classNames;
	std::vector<std::string> enumNames;

	for(const auto & [typeName, registar] : types)
	{
		knownTypes.insert(typeName);
		classNames.push_back(typeName);

		FieldDocRegistrar fieldSink;
		registar->collectFields(fieldSink);
		for(const auto & group : fieldSink.getEnumGroups())
		{
			knownTypes.insert(group.name);
			enumNames.push_back(group.name);
		}
	}

	// Single LuaLS stub aggregating every type — kept as one file so modders can register
	// it as a `Lua.workspace.library` path and get autocomplete across the whole API.
	std::ofstream lua((outDir / "api.lua").string());
	lua << "---@meta\n"
	    << "-- Auto-generated Lua Language Server definitions for the VCMI scripting API.\n"
	    << "-- Generated by `vcmiserver --export-lua-docs`. Do not edit by hand.\n\n";

	for(const auto & [typeName, registar] : types)
	{
		DocRegistrar methodSink;
		registar->collectDocs(methodSink);

		FieldDocRegistrar fieldSink;
		registar->collectFields(fieldSink);

		const auto description = registar->getDescription();
		const auto & methods = methodSink.get();
		const auto & fields = fieldSink.get();
		const auto & enumGroups = fieldSink.getEnumGroups();

		emitClassFile(outDir, typeName, description, methods, fields, enumGroups, knownTypes);
		emitLualsType(lua, typeName, description, methods, fields, enumGroups);

		for(const auto & group : enumGroups)
			emitEnumFile(outDir, group);
	}

	emitIndex(outDir, classNames, enumNames);
}

}

VCMI_LIB_NAMESPACE_END
