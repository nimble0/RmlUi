/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef ROCKETCOREPROPERTYSPECIFICATION_H
#define ROCKETCOREPROPERTYSPECIFICATION_H

#include "Header.h"
#include "Element.h"
#include "PropertyDefinition.h"

namespace Rocket {
namespace Core {

class StyleSheetSpecification;
class PropertyDictionary;
struct ShorthandDefinition;



template <typename ID>
class IdNameMap {
	std::vector<String> name_map;  // IDs are indices into the name_map
	UnorderedMap<String, ID> reverse_map;

protected:
	IdNameMap(size_t num_ids_to_reserve) {
		static_assert((int)ID::Invalid == 0, "Invalid id must be zero");
		name_map.resize(num_ids_to_reserve);
		reverse_map.reserve(num_ids_to_reserve);
		AddPair(ID::Invalid, "invalid");
	}

public:
	void AddPair(ID id, const String& name) {
		// Should only be used for defined IDs
		ROCKET_ASSERT((size_t)id < name_map.size());
		name_map[(size_t)id] = name;
		bool inserted = reverse_map.emplace(name, id).second;
		ROCKET_ASSERT(inserted);
	}

	void AssertAllInserted(ID last_property_inserted) const {
		ptrdiff_t cnt = std::count_if(name_map.begin(), name_map.end(), [](const String& name) { return !name.empty(); });
		ROCKET_ASSERT(cnt == (ptrdiff_t)last_property_inserted && reverse_map.size() == (size_t)last_property_inserted);
	}

	ID GetId(const String& name) const
	{
		auto it = reverse_map.find(name);
		if (it != reverse_map.end())
			return it->second;
		return ID::Invalid;
	}
	const String& GetName(ID id)
	{
		if (static_cast<size_t>(id) < name_map.size())
			return name_map[static_cast<size_t>(id)];
		return name_map[static_cast<size_t>(ID::Invalid)];
	}

	ID GetOrCreateId(const String& name)
	{
		ID next_id = static_cast<ID>(name_map.size());

		// Only insert if not already in list
		auto pair = reverse_map.emplace(name, next_id);
		const auto& it = pair.first;
		bool inserted = pair.second;

		if (inserted)
			name_map.push_back(name);

		// Return the property id that already existed, or the new one if inserted
		return it->second;
	}
};

class PropertyIdNameMap : public IdNameMap<PropertyId> {
public:
	PropertyIdNameMap(size_t reserve_num_properties) : IdNameMap(reserve_num_properties) {}
};

class ShorthandIdNameMap : public IdNameMap<ShorthandId> {
public:
	ShorthandIdNameMap(size_t reserve_num_shorthands) : IdNameMap(2 * (size_t)ShorthandId::NumDefinedIds) {}
};



enum class ShorthandType
{
	// Normal; properties that fail to parse fall-through to the next until they parse correctly, and any
	// undeclared are not set.
	FallThrough,
	// A single failed parse will abort, and any undeclared are replicated from the last declared property.
	Replicate,
	// For 'padding', 'margin', etc; up to four properties are expected.
	Box,
	// Recursively resolves the full value string on each property, whether it is a normal property or another shorthand.
	Recursive
};

enum class ShorthandItemType { Invalid, Property, Shorthand };
struct ShorthandItemId {
	ShorthandItemId() : type(ShorthandItemType::Invalid) {}
	ShorthandItemId(PropertyId id) : type(ShorthandItemType::Property), property_id(id) {}
	ShorthandItemId(ShorthandId id) : type(ShorthandItemType::Shorthand), shorthand_id(id) {}

	ShorthandItemType type;
	union {
		PropertyId property_id;
		ShorthandId shorthand_id;
	};
};
using ShorthandItemIdList = std::vector<ShorthandItemId>;

/**
	A property specification stores a group of property definitions.

	@author Peter Curry
 */

class ROCKETCORE_API PropertySpecification
{
public:
	PropertySpecification(size_t reserve_num_properties, size_t reserve_num_shorthands);
	~PropertySpecification();

	/// Registers a property with a new definition.
	/// @param[in] property_name The name to register the new property under.
	/// @param[in] default_value The default value to be used for an element if it has no other definition provided.
	/// @param[in] inherited True if this property is inherited from parent to child, false otherwise.
	/// @param[in] forces_layout True if this property requires its parent to be reformatted if changed.
	/// @param[in] id If 'Invalid' then automatically assigns a new id, otherwise assigns the given id.
	/// @return The new property definition, ready to have parsers attached.
	PropertyDefinition& RegisterProperty(const String& property_name, const String& default_value, bool inherited, bool forces_layout, PropertyId id = PropertyId::Invalid);
	/// Returns a property definition.
	/// @param[in] id The id of the desired property.
	/// @return The appropriate property definition if it could be found, NULL otherwise.
	const PropertyDefinition* GetProperty(PropertyId id) const;
	const PropertyDefinition* GetProperty(const String& property_name) const;

	/// Returns the list of the names of all registered property definitions.
	/// @return The list with stored property names.
	const PropertyNameList& GetRegisteredProperties() const;

	/// Returns the list of the names of all registered inherited property definitions.
	/// @return The list with stored property names.
	const PropertyNameList& GetRegisteredInheritedProperties() const;

	/// Registers a shorthand property definition.
	/// @param[in] shorthand_name The name to register the new shorthand property under.
	/// @param[in] properties A comma-separated list of the properties this definition is shorthand for. The order in which they are specified here is the order in which the values will be processed.
	/// @param[in] type The type of shorthand to declare.
	/// @param[in] id If 'Invalid' then automatically assigns a new id, otherwise assigns the given id.
	/// @param True if all the property names exist, false otherwise.
	bool RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type, ShorthandId id = ShorthandId::Invalid);
	/// Returns a shorthand definition.
	/// @param[in] shorthand_name The name of the desired shorthand.
	/// @return The appropriate shorthand definition if it could be found, NULL otherwise.
	const ShorthandDefinition* GetShorthand(ShorthandId id) const;
	const ShorthandDefinition* GetShorthand(const String& shorthand_name) const;

	/// Parse declaration by name, whether its a property or shorthand.
	bool ParsePropertyDeclaration(PropertyDictionary& dictionary, const String& property_name, const String& property_value, const String& source_file = "", int source_line_number = 0) const;

	bool ParsePropertyDeclaration(PropertyDictionary& dictionary, PropertyId property_id, const String& property_value, const String& source_file = "", int source_line_number = 0) const;

	/// Parses a property declaration, setting any parsed and validated properties on the given dictionary.
	/// @param dictionary The property dictionary which will hold all declared properties.
	/// @param property_name The name of the declared property.
	/// @param property_value The values the property is being set to.
	/// @return True if all properties were parsed successfully, false otherwise.
	bool ParseShorthandDeclaration(PropertyDictionary& dictionary, ShorthandId shorthand_id, const String& property_value, const String& source_file = "", int source_line_number = 0) const;
	/// Sets all undefined properties in the dictionary to their defaults.
	/// @param dictionary[in] The dictionary to set the default values on.
	void SetPropertyDefaults(PropertyDictionary& dictionary) const;

private:
	typedef std::vector< PropertyDefinition* > Properties;
	typedef std::vector< ShorthandDefinition* > Shorthands;

	Properties properties;
	Shorthands shorthands;

	PropertyIdNameMap property_map;
	ShorthandIdNameMap shorthand_map;

	PropertyNameList property_names;
	PropertyNameList inherited_property_names;

	bool ParsePropertyValues(StringList& values_list, const String& values, bool split_values) const;

	friend class StyleSheetSpecification;
};

}
}

#endif
