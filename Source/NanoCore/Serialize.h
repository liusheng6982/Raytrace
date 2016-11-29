#ifndef ___INC_NANOCORE_SERIALIZE
#define ___INC_NANOCORE_SERIALIZE

#include <vector>
#include <string>
#include "File.h"


namespace NanoCore {

class XmlNode {
public:
	XmlNode();
	XmlNode( const char * name );
	~XmlNode();

	template< typename T >
	void SerializeAttrib( const char * name, T & x ) {
		if( !GetAttribute( name, x ))
			SetAttribute( name, x );
	}
	XmlNode * SerializeChild( const char * name );

	const char * GetName() const;
	const char * GetValue() const;
	bool GetAttribute( const char * name, float & f );
	bool GetAttribute( const char * name, int & i );
	bool GetAttribute( const char * name, std::string & s );
	int  GetAttributeByName( const char * name );
	int  GetNumAttributes();
	const char * GetAttributeName( int idx );
	const char * GetAttributeValue( int idx );

	bool Get( float & f );
	bool Get( int & i );
	bool Get( std::string & s );
	void Set( float f );
	void Set( int i );
	void Set( const char * str );

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



struct KeyValuePtr {
	std::string name;
	int * i;
	float * f;
	std::string * str;

	KeyValuePtr(){}
	KeyValuePtr( const char * name, int & i ) : name(name), i(&i), f(0), str(0) {}
	KeyValuePtr( const char * name, float & f ) : name(name), i(0), f(&f), str(0) {}
	KeyValuePtr( const char * name, std::string & str ) : name(name), i(0), f(0), str(&str) {}

	std::string GetNameAsTag() const;

	void SetValue( const char * pc );
	std::string GetValue() const;

	void Serialize( XmlNode * pRoot );
};

template< typename T > void Serialize( XmlNode * pRoot, const char * tag, std::vector<T> & v ) {
	XmlNode * p = pRoot->GetChild( tag );
	if( p ) {
		v.resize( p->GetNumChildren() );
		for( int i=0; i<p->GetNumChildren(); ++i ) {
			XmlNode * item = p->GetChild( i );
			v[i].Serialize( item );
		}
	} else {
		p = new XmlNode( tag );
		pRoot->AddChild( p );
		for( size_t i=0; i<v.size(); ++i ) {
			XmlNode * item = new XmlNode( "item" );
			p->AddChild( item );
			v[i].Serialize( item );
		}
	}
}

void Serialize( XmlNode * pRoot, const char * tag, std::vector<KeyValuePtr> & v );

}
#endif