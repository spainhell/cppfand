#include "Record.h"

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

Record::Record(FileD* file_d, uint8_t* record)
{
	_file_d = file_d;
	_buffer = record;
	_delete_record_on_destroy = false;
}

Record::~Record()
{
	if (_delete_record_on_destroy) {
		delete[] _buffer;
		_buffer = nullptr;
	}
}

Record* Record::Clone() const
{
	Record* clone = new Record(_file_d);
	clone->_values = this->_values;
	clone->_updated = this->_updated;
	return clone;
}

uint8_t* Record::GetRecord() const
{
	return _buffer;
}

void Record::Reset()
{
	for (BRS_Value& val : _values) {
		val.Reset();
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
}

void Record::ClearDeleted()
{
	_deleted = false;
}

bool Record::IsDeleted() const
{
	return _deleted;
}

uint8_t* Record::PrepareRecord()
{
	_setRecordFromValues();
	return _buffer;
}

void Record::Expand()
{
	_values = _getValuesFromRecord();
}

std::vector<BRS_Value> Record::_getValuesFromRecord()
{
	std::vector<BRS_Value> values;
	for (FieldDescr* field : _file_d->FldD)
	{
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
		} else {
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

		values.push_back(val);
	}
	return values;
}

void Record::_setRecordFromValues()
{
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
