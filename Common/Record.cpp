#include "Record.h"

#include <stdexcept>
#include <utility>

Record::Record()
{
	_file_d = nullptr;
}

Record::Record(FileD* file_d)
{
	_file_d = file_d;

	for (FieldDescr* field_d : file_d->FldD) {
		BRS_Value val;
		_values.push_back(val);
	}
}

Record::~Record()
{
}

void Record::CopyTo(Record* dst_record) const
{
	// memcpy(dst_record->_buffer, this->_buffer, this->_file_d->GetRecLen());
	dst_record->_values = this->_values;
	dst_record->_updated = this->_updated;
	dst_record->_deleted = this->_deleted;
}

Record* Record::Clone() const
{
	Record* clone = new Record(_file_d);
	clone->_values = this->_values;
	clone->_updated = this->_updated;
	clone->_deleted = this->_deleted;
	return clone;
}

void Record::Clear()
{
	_updated = false;
	_deleted = false;
	_values.clear();
}

FileD* Record::GetFileD() const
{
	return _file_d;
}

void Record::Reset()
{
	_updated = false;
	_deleted = false;
	for (BRS_Value& val : _values) {
		val.Reset();
	}
}

bool Record::LoadB(const std::string& field_name) const
{
	size_t index = _getFieldDescrIndexByName(field_name);
	if (std::cmp_not_equal(index, -1)) {
		return _values[index].B;
	} 
	else {
		return false;
	}
}

double Record::LoadR(const std::string& field_name) const
{
	size_t index = _getFieldDescrIndexByName(field_name);
	if (std::cmp_not_equal(index, -1)) {
		return _values[index].R;
	}
	else {
		return 0.0;
	}
}

std::string Record::LoadS(const std::string& field_name) const
{
	size_t index = _getFieldDescrIndexByName(field_name);
	if (std::cmp_not_equal(index, -1)) {
		return _values[index].S;
	}
	else {
		return "";
	}
}

void Record::SaveB(const std::string& field_name, bool value)
{
	size_t index = _getFieldDescrIndexByName(field_name);
	if (std::cmp_not_equal(index, -1)) {
		_values[index].B = value;
		SetUpdated();
	}
}

void Record::SaveR(const std::string& field_name, double value)
{
	size_t index = _getFieldDescrIndexByName(field_name);
	if (std::cmp_not_equal(index, -1)) {
		_values[index].R = value;
		SetUpdated();
	}
}

void Record::SaveS(const std::string& field_name, const std::string& value)
{
	size_t index = _getFieldDescrIndexByName(field_name);
	if (std::cmp_not_equal(index, -1)) {
		_values[index].S = value;
		SetUpdated();
	}
}

void Record::SetUpdated()
{
	_updated = true;
}

void Record::ClearUpdated()
{
	_updated = false;
}

bool Record::IsUpdated() const
{
	return _updated;
}

void Record::SetDeleted(bool set_updated)
{
	_deleted = true;
	if (set_updated) {
		SetUpdated();
	}
}

void Record::ClearDeleted()
{
	_deleted = false;
	SetUpdated();
}

bool Record::IsDeleted() const
{
	return _deleted;
}

uint8_t Record::Compare(Record* rec1, Record* rec2)
{
	if (rec1->_file_d != rec2->_file_d) {
		return 0x2;
	}
	for (size_t i = 0; i < rec1->_file_d->FldD.size(); i++) {
		FieldDescr* field = rec1->_file_d->FldD[i];
		if (field->isStored()) {
			const BRS_Value& val1 = rec1->_values[i];
			const BRS_Value& val2 = rec2->_values[i];
			switch (field->field_type) {
			case FieldType::BOOL:
				if (val1.B != val2.B) return 0x2;
				break;
			case FieldType::FIXED:
			case FieldType::REAL:
				if (val1.R != val2.R) return 0x2;
				break;
			case FieldType::ALFANUM:
			case FieldType::NUMERIC:
			case FieldType::TEXT:
				if (val1.S != val2.S) return 0x2;
				break;
			default:
				// unknown field type
				break;
			}
		}
	}
	return 0x1; // records are equal
}

FieldDescr* Record::_getFieldDescrByName(const std::string& field_name) const
{
	for (FieldDescr* field : _file_d->FldD) {
		if (field->Name == field_name) {
			return field;
		}
	}
	return nullptr; // field not found
}

size_t Record::_getFieldDescrIndexByName(const std::string& field_name) const
{
	for (size_t i = 0; i < _file_d->FldD.size(); i++) {
		if (_file_d->FldD[i]->Name == field_name) {
			return i;
		}
	}
	return static_cast<size_t>(-1); // field not found
}
