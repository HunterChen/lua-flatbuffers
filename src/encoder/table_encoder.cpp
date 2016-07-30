#include "table_encoder.h"

// Todo: check required fields.
// Todo: Skip default value.

flatbuffers::uoffset_t TableEncoder::EncodeTable(
	const Object& obj, const LuaRef& luaTable)
{
	assert(!obj.is_struct());

	// Cache to map before StartTable().
	m_mapStruct.clear();
	m_mapScalar.clear();
	m_mapOffset.clear();
	if (!CacheFields(obj, luaTable))
		return 0;

	uoffset_t start = Builder().StartTable();
	EncodeCachedStructs();
	EncodeCachedScalars();
	EncodeCachedOffsets();
	return Builder().EndTable(start, obj.fields()->size());
}

flatbuffers::uoffset_t TableEncoder::EncodeVector(
	const reflection::Type& type, const LuaRef& luaArray)
{
	assert(type.base_type() == reflection::Vector);
	// todo: check luaArray is array
	return 0;
}

// Cache fields to 3 maps.
bool TableEncoder::CacheFields(const Object& obj, const LuaRef& luaTable)
{
	const auto& vFields = *obj.fields();
	for (const auto& e : luaTable)
	{
		string sKey = e.key<string>();
		const Field* pField = vFields.LookupByKey(sKey.c_str());
		if (!CheckObjectField(pField, sKey))
			return false;
		assert(pField);

		LuaRef value = e.value<LuaRef>();
		CacheField(pField, value);
	}
	return true;
}

// Cache field to 3 maps.
void TableEncoder::CacheField(const Field* pField, const LuaRef& luaValue)
{
	assert(pField);
	using namespace reflection;
	const Type& type = *pField->type();
	// Todo: check type of value...
	switch (type.base_type())
	{
	case String:
		m_mapOffset[pField] = Builder().CreateString(
			luaValue.toValue<const char*>()).o;
		break;
	case Vector:
		m_mapOffset[pField] = EncodeVector(type, luaValue);
		break;
	case Obj:
	{
		const Object* pObj = Objects()[type.index()];
		assert(pObj);
		if (pObj->is_struct())
			m_mapStruct[pField] = luaValue;
		else
			m_mapOffset[pField] = TableEncoder(m_rCtx)
				.EncodeTable(*pObj, luaValue);
		break;
	}
	case Union:
		// XXX get union underlying type...
		break;
	default:
		m_mapScalar[pField] = luaValue;
		break;
	}  // switch
}

void TableEncoder::EncodeCachedStructs()
{
	// XXX
}

void TableEncoder::EncodeCachedScalars()
{
	for (const auto& e : m_mapScalar)
	{
		const Field* pField = e.first;
		assert(pField);
		EncodeScalar(*pField, e.second);
	}
}

void TableEncoder::EncodeScalar(const Field& field, const LuaRef& elementValue)
{
	using namespace reflection;
	const Type& type = *field.type();
	int64_t defInt = field.default_integer();
	double defReal = field.default_real();
	uint16_t offset = field.offset();

	assert(type.base_type() <= Double);  // Only scalar.
	switch (type.base_type())
	{
	case UType:
	case Bool:
	case UByte:
		AddElement<uint8_t>(offset, elementValue, defInt);
		break;
	case Byte:
		AddElement<int8_t>(offset, elementValue, defInt);
		break;
	case Short:
		AddElement<int16_t>(offset, elementValue, defInt);
		break;
	case UShort:
		AddElement<uint16_t>(offset, elementValue, defInt);
		break;
	case Int:
		AddElement<int32_t>(offset, elementValue, defInt);
		break;
	case UInt:
		AddElement<uint32_t>(offset, elementValue, defInt);
		break;
	case Long:
		AddElement<int64_t>(offset, elementValue, defInt);
		break;
	case ULong:
		AddElement<uint64_t>(offset, elementValue, defInt);
		break;
	case Float:
		AddElement<float>(offset, elementValue, defReal);
		break;
	case Double:
		AddElement<double>(offset, elementValue, defReal);
		break;
	default:
		assert(!"Illegal type.");
		break;
	}
}

void TableEncoder::EncodeCachedOffsets()
{
	for (const auto& e : m_mapOffset)
	{
		const Field* pField = e.first;
		assert(pField);
		uoffset_t offset = e.second;
		Builder().AddOffset(pField->offset(), flatbuffers::Offset<void>(offset));
	}
}

template <typename ElementType, typename DefaultValueType>
inline void TableEncoder::AddElement(uint16_t offset,
	const LuaRef& elementValue, DefaultValueType defaultValue)
{
	Builder().AddElement(offset,
		elementValue.toValue<ElementType>(),
		static_cast<ElementType>(defaultValue));
}

// Set error and return false if field is illegal.
// sFieldName is only used for error message.
bool TableEncoder::CheckObjectField(const Field* pField, const string& sFieldName)
{
	if (!pField)
		ERR_RET_FALSE("illegal field " + PopFullFieldName(sFieldName));
	if (pField->deprecated())
		ERR_RET_FALSE("deprecated field " + PopFullFieldName(sFieldName));
	return true;
}