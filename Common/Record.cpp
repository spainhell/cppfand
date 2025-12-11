#include "Record.h"

#include <utility>

Record::Record()
{
	_file_d = nullptr;
	_buffer = nullptr;
}

Record::Record(FileD* file_d)
{
	_file_d = file_d;
	_buffer = file_d->GetRecSpace();

	for (FieldDescr* field_d : file_d->FldD) {
		BRS_Value val;
		_values.push_back(val);
	}
}

Record::Record(FileD* file_d, uint8_t* record, bool record_owner)
{
	_file_d = file_d;
	_buffer = record;
	_delete_record_on_destroy = record_owner;
	_getValuesFromRecord();
}

Record::~Record()
{
	if (_delete_record_on_destroy) {
		delete[] _buffer;
		_buffer = nullptr;
	}
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
	memcpy(clone->_buffer, this->_buffer, this->_file_d->GetRecLen());
	clone->_values = this->_values;
	clone->_updated = this->_updated;
	clone->_deleted = this->_deleted;
	return clone;
}

uint8_t* Record::GetRecord()
{
	_setRecordFromValues();
	return _buffer;
}

FileD* Record::GetFileD() const
{
	return _file_d;
}

void Record::Reset()
{
	for (BRS_Value& val : _values) {
		val.Reset();
	}
}

void Record::Expand()
{
	_getValuesFromRecord();
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

void Record::SetDeleted()
{
	_deleted = true;
	SetUpdated();
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

void Record::_getValuesFromRecord()
{
	_values.clear();

	if (_buffer != nullptr) {
		for (FieldDescr* field : _file_d->FldD) {
			BRS_Value val;

			if (field->isStored()) {
				switch (field->field_type) {
				case FieldType::BOOL:
					val.B = _file_d->loadB(field, _buffer);
					break;
				case FieldType::DATE:
				case FieldType::FIXED:
				case FieldType::REAL:
					val.R = _file_d->loadR(field, _buffer);
					break;
				case FieldType::ALFANUM:
				case FieldType::NUMERIC:
				case FieldType::TEXT:
					val.S = _file_d->loadS(field, _buffer);
					break;
				default:
					// unknown field type
					break;
				}
			}
			else {
				// TODO: calculated T fields? is it stored in TWork by default?

				// calculated field
				//switch (field->field_type) {
				//case FieldType::BOOL:
				//	val.B = RunBool(_file_d, field->Frml, _buffer);
				//	break;
				//case FieldType::DATE:
				//case FieldType::FIXED:
				//case FieldType::REAL:
				//	val.R = RunReal(_file_d, field->Frml, _buffer);
				//	break;
				//case FieldType::ALFANUM:
				//case FieldType::NUMERIC:
				//case FieldType::TEXT:
				//	val.S = RunString(_file_d, field->Frml, _buffer);
				//	break;
				//default:
				//	// unknown field type
				//	break;
				//}
			}

			_values.push_back(val);
		}
	}
	else
	{
		// buffer is null -> return empty values
	}
}

void Record::_setRecordFromValues()
{
	if (IsUpdated()) {
		if (_file_d->HasIndexFile() && IsDeleted()) {
			_file_d->SetDeletedFlag(_buffer);
		}
		else {
			_file_d->ClearDeletedFlag(_buffer);
		}

		if (IsUpdated()) {
			_file_d->SetRecordUpdateFlag(_buffer);
		}
		else {
			_file_d->ClearRecordUpdateFlag(_buffer);
		}

		for (size_t i = 0; i < _file_d->FldD.size(); i++) {
			FieldDescr* field = _file_d->FldD[i];

			if (field->isStored()) {
				BRS_Value& val = _values[i];
				switch (field->field_type) {
				case FieldType::BOOL:
					_file_d->saveB(field, val.B, _buffer);
					break;
				case FieldType::DATE:
				case FieldType::FIXED:
				case FieldType::REAL:
					_file_d->saveR(field, val.R, _buffer);
					break;
				case FieldType::ALFANUM:
				case FieldType::NUMERIC:
				case FieldType::TEXT:
					_file_d->saveS(field, val.S, _buffer);
					break;
				default:
					// unknown field type
					break;
				}
			}
			else {
				// calculated field -> do nothing
			}
		}
	}
	else {
		// not updated -> _buffer will not be changed

	}
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
