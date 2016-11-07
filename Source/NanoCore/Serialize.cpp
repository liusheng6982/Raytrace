#include "Serialize.h"

namespace NanoCore {


#define CHECK_RANGE( vec, idx, ret ) if( idx < 0 || idx >= vec.size()) return ret;

XmlNode::XmlNode() {
}

XmlNode::XmlNode( const char * name ) {
	m_Name = name;
}

XmlNode::~XmlNode() {
	for( size_t i=0; i<m_Children.size(); ++i )
		delete m_Children[i];
}

const char * XmlNode::GetName() {
	return m_Name.c_str();
}

int XmlNode::GetAttributeByName( const char * name ) {
	for( size_t i=0, n=m_Attributes.size(); i<n; ++i ) {
		if( m_Attributes[i].first == name )
			return i;
	}
	return -1;
}

bool XmlNode::GetAttribute( const char * name, float & f ) {
	int idx = GetAttributeByName( name );
	if( idx < 0 ) return false;
	f = (float)atof( m_Attributes[idx].second.c_str());
	return true;
}

bool XmlNode::GetAttribute( const char * name, int & i ) {
	int idx = GetAttributeByName( name );
	if( idx < 0 ) return false;
	i = atol( m_Attributes[idx].second.c_str());
	return true;
}

bool XmlNode::GetAttribute( const char * name, std::string & s ) {
	int idx = GetAttributeByName( name );
	if( idx < 0 ) return false;
	s = m_Attributes[idx].second;
	return true;
}

int XmlNode::GetNumAttributes() {
	return m_Attributes.size();
}

const char * XmlNode::GetAttributeName( int idx ) {
	CHECK_RANGE( m_Attributes, idx, NULL );
	return m_Attributes[idx].first.c_str();
}

const char * XmlNode::GetAttributeValue( int idx ) {
	CHECK_RANGE( m_Attributes, idx, NULL );
	return m_Attributes[idx].second.c_str();
}

void XmlNode::Get( float & f ) {
	f = (float)atof(m_Value.c_str());
}

void XmlNode::Get( int & i ) {
	i = atol(m_Value.c_str());
}

void XmlNode::Get( std::string & s ) {
	s = m_Value;
}

int XmlNode::GetNumChildren() {
	return m_Children.size();
}

XmlNode * XmlNode::GetChild( int idx ) {
	CHECK_RANGE( m_Children, idx, NULL );
	return m_Children[idx];
}

XmlNode * XmlNode::GetChild( const char * name ) {
	for( size_t i=0, n=m_Children.size(); i<n; ++i )
		if( m_Children[i]->GetName() == name )
			return m_Children[i];
	return NULL;
}

void XmlNode::SetName( const char * name ) {
	m_Name = name;
}

static void Str( float f, std::string & s ) {
	char buf[64];
	sprintf( buf, "%0.4f", f );
	s = buf;
}

static void Str( int i, std::string & s ) {
	char buf[64];
	sprintf( buf, "%d", i );
	s = buf;
}

void XmlNode::SetAttribute( const char * name, float & f ) {
	int idx = GetAttributeByName( name );
	if( idx > 0 )
		Str( f, m_Attributes[idx].second );
	else {
		m_Attributes.resize( m_Attributes.size()+1 );
		m_Attributes.back().first = name;
		Str( f, m_Attributes.back().second );
	}
}

void XmlNode::SetAttribute( const char * name, int & i ) {
	int idx = GetAttributeByName( name );
	if( idx > 0 )
		Str( i, m_Attributes[idx].second );
	else {
		m_Attributes.resize( m_Attributes.size()+1 );
		m_Attributes.back().first = name;
		Str( i, m_Attributes.back().second );
	}
}

void XmlNode::SetAttribute( const char * name, std::string & s ) {
	int idx = GetAttributeByName( name );
	if( idx > 0 )
		m_Attributes[idx].second  = s;
	else {
		m_Attributes.resize( m_Attributes.size()+1 );
		m_Attributes.back().first = name;
		m_Attributes.back().second = s;
	}
}

void XmlNode::DeleteAttribute( const char * name ) {
	int idx = GetAttributeByName( name );
	if( idx < 0 )
		return;
	m_Attributes.erase( m_Attributes.begin() + idx );
}

void XmlNode::DeleteAttribute( int idx ) {
	m_Attributes.erase( m_Attributes.begin() + idx );
}

void XmlNode::AddChild( XmlNode * node ) {
	m_Children.push_back( node );
}

void XmlNode::DeleteChild( int idx ) {
	m_Children.erase( m_Children.begin() + idx );
}

void XmlNode::Save( TextFile & tf ) {
	if( !GetNumChildren()) {
			tf.Write( "<%s", GetName());
			for( int i=0; i<GetNumAttributes(); ++i )
				tf.Write( " %s=\"%s\"", GetAttributeName( i ), GetAttributeValue( i ));
			tf.Write( "/>\n" );
	} else {
		tf.Write( "<%s>\n", GetName());
		for( int i=0; i<GetNumChildren(); ++i )
			GetChild(i)->Save( tf );
		tf.Write( "</%s>\n", GetName());
	}
}

void XmlNode::Load( TextFile & tf ) {
	char buf[512];
	tf.ReadLine( buf, 512 );

	char * p = strchr( buf, '<' );
	char * t = strchr( p, ' ' );
	*t = 0;

	SetName( p+1 );


}



}