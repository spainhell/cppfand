#include "Record.h"

#include <stdexcept>
#include <utility>
#include "../Core/runfrml.h"


Record::Record(FileD* file_d)
{
	_file_d = file_d;

	for (FieldDescr* field_d : file_d->FldD) {
		BRS_Value val;
		_values.insert(std::pair(field_d->Name, val));
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
	for (std::pair<const std::string, BRS_Value>& val : _values) {
		val.second.Reset();
	}
}

bool Record::LoadB(FieldDescr* field) const
{
	if (field->isStored()) {
		std::map<std::string, BRS_Value>::const_iterator item = _values.find(field->Name);
		if (item != _values.end()) {
			return item->second.B;
		}
		else {
			return false;
		}
	}
	else {
		return RunBool(this->_file_d, field->Frml, const_cast<Record*>(this));
	}
}

double Record::LoadR(FieldDescr* field) const
{
	if (field->isStored()) {
		std::map<std::string, BRS_Value>::const_iterator item = _values.find(field->Name);
		if (item != _values.end()) {
			return item->second.R;
		}
		else {
			return 0.0;
		}
	}
	else {
		return RunReal(this->_file_d, field->Frml, const_cast<Record*>(this));
	}
}

std::string Record::LoadS(FieldDescr* field)
{
	std::string result;

	if (field->isStored()) {
		std::map<std::string, BRS_Value>::iterator item = _values.find(field->Name);
		if (item != _values.end()) {
			if (item->second.R == 0.0) {
				return item->second.S;
			}
			else {
				if (_file_d->FileType == DataFileType::FandFile) {
					// T field stored as position in .T__ file
					// need to load text from .T__ file
					int pos = static_cast<int>(item->second.R);
					item->second.S = _file_d->FF->loadTfromPos(field, pos);
					// set R to 0.0 to indicate that text is loaded
					item->second.R = 0.0;
					result = item->second.S;
				}
				else {
					throw std::runtime_error("Lazy loading T field from non-FandFile is not supported.");
				}
			}
		}
		else {
			// return result (empty string)
		}
	}
	else {
		result = RunString(this->_file_d, field->Frml, this);
	}

	return result;
}

void Record::SaveB(FieldDescr* field, bool value)
{
	if (field->isStored()) {
		std::map<std::string, BRS_Value>::iterator item = _values.find(field->Name);
		if (item != _values.end()) {
			item->second.B = value;
			SetUpdated();
		}
	}
}

void Record::SaveR(FieldDescr* field, double value)
{
	if (field->isStored()) {
		std::map<std::string, BRS_Value>::iterator item = _values.find(field->Name);
		if (item != _values.end()) {
			item->second.R = value;
			SetUpdated();
		}
	}
}

void Record::SaveS(FieldDescr* field, const std::string& value)
{
	if (field->isStored()) {
		std::map<std::string, BRS_Value>::iterator item = _values.find(field->Name);
		if (item != _values.end()) {
			item->second.R = 0.0; // indicates that text is stored in S
			item->second.S = value;
			SetUpdated();
		}
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
			std::map<std::string, BRS_Value>::const_iterator itemA = rec1->_values.find(field->Name);
			if (itemA == rec1->_values.end()) {
				return 0x2;
			}
			
			std::map<std::string, BRS_Value>::const_iterator itemB = rec2->_values.find(field->Name);
			if (itemB == rec2->_values.end()) {
				return 0x2;
			}

			switch (field->field_type) {
			case FieldType::BOOL:
				if (itemA->second.B != itemB->second.B) return 0x2;
				break;
			case FieldType::FIXED:
			case FieldType::REAL:
				if (itemA->second.R != itemB->second.R) return 0x2;
				break;
			case FieldType::ALFANUM:
			case FieldType::NUMERIC:
			case FieldType::TEXT:
				if (itemA->second.S != itemB->second.S) return 0x2;
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
