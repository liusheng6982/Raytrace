#ifndef ___INC_NANOCORE_SERIALIZE
#define ___INC_NANOCORE_SERIALIZE

#include <vector>
#include <string>
#include "File.h"


namespace NanoCore {

int PatternMatch( const char * buffer, const char * match, ... );

class XmlNode {
public:
	enum EDataType {
		edtFloat,
		edtInt,
		edtString
	};

	XmlNode();
	XmlNode( const char * name );
	~XmlNode();

	void SerializeAttrib( const char * name, float & f ) {
		if( !GetAttribute( name, f ))
			SetAttribute( name, f );
	}
	XmlNode * SerializeChild( const char * name ) {
		XmlNode * p = GetChild( name );
		if( p )
			return p;
		p = new XmlNode( name );
		AddChild( p );
		return p;
	}

	const char * GetName();
	bool GetAttribute( const char * name, float & f );
	bool GetAttribute( const char * name, int & i );
	bool GetAttribute( const char * name, std::string & s );
	int  GetAttributeByName( const char * name );
	int  GetNumAttributes();
	const char * GetAttributeName( int idx );
	const char * GetAttributeValue( int idx );

	void Get( float & f );
	void Get( int & i );
	void Get( std::string & s );

	int  GetNumChildren();
	XmlNode * GetChild( int idx );
	XmlNode * GetChild( const char * name );

	void SetName( const char * name );
	void SetAttribute( const char * name, float & f );
	void SetAttribute( const char * name, int & i );
	void SetAttribute( const char * name, const char * s );
	void DeleteAttribute( const char * name );
	void DeleteAttribute( int idx );

	void AddChild( XmlNode * node );
	void DeleteChild( int idx );

	void Save( TextFile & tf );
	bool Load( TextFile & tf );

private:
	bool Load( TextFile & tf, const char * firstLine );

	std::string m_Name, m_Value;
	std::vector<std::pair<std::string,std::string>> m_Attributes;
	std::vector<XmlNode*> m_Children;
};



}
#endif