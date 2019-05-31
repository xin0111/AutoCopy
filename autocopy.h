#ifndef autocopy_H__
#define autocopy_H__

#include <QList>
#include <QAtomicInt>
#include <QString>
#include <QVariant>
#include <QMetaType> 
/// struct to represent cmake properties in Qt
/// Value is of type String or Bool
struct AutoCopyProperty
{
	enum PropertyType
	{
		BOOL,
		PATH,
		FILE,
		FILE_PATH,
		STRING
	};
	QString Key;
	PropertyType KeyType;
	QVariant Value;
	PropertyType ValueType;
	QStringList Strings;
	QString Help;
	bool Advanced;
	bool operator==(const AutoCopyProperty& other) const
	{
		return this->Key == other.Key;
	}
	bool operator<(const AutoCopyProperty& other) const
	{
		return this->Key < other.Key;
	}
};

// list of properties
typedef QList<AutoCopyProperty> AutoCopyPropertyList;


// allow QVariant to be a property or list of properties
Q_DECLARE_METATYPE(AutoCopyProperty)
Q_DECLARE_METATYPE(AutoCopyPropertyList)


#endif // autocopy_H__
