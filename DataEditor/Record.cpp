#include "Record.h"

Record::Record(FileD* file_d)
{
	_file_d = file_d;
	_record = file_d->GetRecSpace();

	for (FieldDescr* field_d : file_d->FldD) {
		BRS_Value val;
		_values.push_back(val);
	}
}

Record::Record(FileD* file_d, uint8_t* record)
{
	_file_d = file_d;
	_record = record;
	_delete_record_on_destroy = false;
}

Record::~Record()
{
	if (_delete_record_on_destroy) {
		delete[] _record;
		_record = nullptr;
	}
}

Record* Record::Clone() const
{
	Record* clone = new Record(_file_d);
	clone->_values = this->_values;
	clone->_was_updated = this->_was_updated;
	return clone;
}

uint8_t* Record::GetRecord() const
{
	return _record;
}

void Record::Reset()
{
	for (BRS_Value& val : _values) {
		val.Reset();
	}
}

void Record::SetRecordUpdateFlag()
{
	_was_updated = true;
}

uint8_t* Record::PrepareRecord()
{
	_setRecordFromValues();
	return _record;
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
				val.B = _file_d->loadB(field, _record);
				break;
			case FieldType::DATE:
			case FieldType::FIXED:
			case FieldType::REAL:
				val.R = _file_d->loadR(field, _record);
				break;
			case FieldType::ALFANUM:
			case FieldType::NUMERIC:
			case FieldType::TEXT:
				val.S = _file_d->loadS(field, _record);
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
			//	val.B = RunBool(_file_d, field->Frml, _record);
			//	break;
			//case FieldType::DATE:
			//case FieldType::FIXED:
			//case FieldType::REAL:
			//	val.R = RunReal(_file_d, field->Frml, _record);
			//	break;
			//case FieldType::ALFANUM:
			//case FieldType::NUMERIC:
			//case FieldType::TEXT:
			//	val.S = RunString(_file_d, field->Frml, _record);
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
				_file_d->saveB(field, val.B, _record);
				break;
			case FieldType::DATE:
			case FieldType::FIXED:
			case FieldType::REAL:
				_file_d->saveR(field, val.R, _record);
				break;
			case FieldType::ALFANUM:
			case FieldType::NUMERIC:
			case FieldType::TEXT:
				_file_d->saveS(field, val.S, _record);
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
