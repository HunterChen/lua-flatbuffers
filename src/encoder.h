#ifndef LUA_FLATBUFFERS_ENCODER_H_
#define LUA_FLATBUFFERS_ENCODER_H_

#include "name_stack.h"  // for NameStack

#include <flatbuffers/flatbuffers.h>

#include <unordered_map>

namespace LuaIntf {
class LuaRef;
}

namespace reflection {
struct Field;
struct Object;
struct Schema;
struct Type;
}

class Encoder final
{
public:
	explicit Encoder(const reflection::Schema& schema);

public:
	using LuaRef = LuaIntf::LuaRef;
	using string = std::string;
	bool Encode(const string& sName, const LuaRef& luaTable);
	string GetResultStr() const;
	const string& GetErrorStr() const { return m_sError; }

private:
	// Encode recursively. Return 0 and set m_sError if any error.
	using Object = reflection::Object;
	using uoffset_t = flatbuffers::uoffset_t;
	using Field = reflection::Field;
	uoffset_t EncodeObject(const Object& obj, const LuaRef& luaTable);

	uoffset_t EncodeStruct(const Object& obj, const LuaRef& luaTable);
	bool EncodeStructToBuf(const Object& obj,
		const LuaRef& luaTable, uint8_t* pBuf);
	bool EncodeStructFieldToBuf(const Field& field,
		const LuaRef& luaTable, uint8_t* pBuf);

	uoffset_t EncodeTable(const Object& obj, const LuaRef& luaTable);
	uoffset_t EncodeVector(const reflection::Type& type, const LuaRef& luaArray);

	// Cache to map before StartTable().
	using Field2Scalar = std::unordered_map<const Field*, LuaRef>;
	using Field2Offset = std::unordered_map<const Field*, uoffset_t>;
	bool CacheFields(const Object& obj, const LuaRef& luaTable,
		Field2Scalar& rMapScalar, Field2Offset& rMapOffset);
	void CacheField(const Field* pField, const LuaRef& luaValue,
		Field2Scalar& rMapScalar, Field2Offset& rMapOffset);

	void AddElements(const Field2Scalar& mapScalar);
	void AddOffsets(const Field2Offset& mapOffset);
	void AddElement(const Field& field, const LuaRef& value);

	template <typename ElementType, typename DefaultValueType>
	inline void AddElement(uint16_t offset, const LuaRef& elementValue,
		DefaultValueType defaultValue);

private:
	string PopFullFieldName(const string& sFieldName);
	void Reset();
	bool CheckObjectField(const Field* pField, const string& sFieldName);

private:
	flatbuffers::FlatBufferBuilder m_fbb;
	const reflection::Schema& m_schema;
	const flatbuffers::Vector<flatbuffers::Offset<Object>>& m_vObjects;
	NameStack m_nameStack;  // For error message.
	string m_sError;  // Encode error.
};  // class Encoder

#endif  // LUA_FLATBUFFERS_ENCODER_H_
