#include "Serialize.h"
#include "String.h"



namespace NanoCore {



#define CHECK_RANGE( vec, idx, ret ) if( idx < 0 || idx >= (int)vec.size()) return ret;

XmlNode::XmlNode() {
}

XmlNode::XmlNode( const char * name ) {
	m_Name = name;
}

XmlNode::~XmlNode() {
	for( size_t i=0; i<m_Children.size(); ++i )
		delete m_Children[i];
}

const char * XmlNode::GetName() const {
	return m_Name.c_str();
}

const char * XmlNode::GetValue() const {
	return m_Value.c_str();
}

int XmlNode::GetAttributeByName( const char * name ) {
	for( size_t i=0, n=m_Attributes.size(); i<n; ++i ) {
		if( m_Attributes[i].first == name )
			return (int)i;
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
	return (int)m_Attributes.size();
}

const char * XmlNode::GetAttributeName( int idx ) {
	CHECK_RANGE( m_Attributes, idx, NULL );
	return m_Attributes[idx].first.c_str();
}

const char * XmlNode::GetAttributeValue( int idx ) {
	CHECK_RANGE( m_Attributes, idx, NULL );
	return m_Attributes[idx].second.c_str();
}

bool XmlNode::Get( float & f ) {
	if( m_Value.empty())
		return false;
	f = (float)atof(m_Value.c_str());
	return true;
}

bool XmlNode::Get( int & i ) {
	if( m_Value.empty())
		return false;
	i = atol(m_Value.c_str());
	return true;
}

bool XmlNode::Get( std::string & s ) {
	if( m_Value.empty())
		return false;
	s = m_Value;
	return true;
}

void XmlNode::Set( float f ) {
	char buf[16];
	sprintf_s( buf, "%0.4f", f );
	m_Value = buf;
}

void XmlNode::Set( int i ) {
	char buf[16];
	sprintf_s( buf, "%d", i );
	m_Value = buf;
}

void XmlNode::Set( const char * str ) {
	m_Value = str;
}

int XmlNode::GetNumChildren() {
	return (int)m_Children.size();
}

XmlNode * XmlNode::GetChild( int idx ) {
	CHECK_RANGE( m_Children, idx, NULL );
	return m_Children[idx];
}

XmlNode * XmlNode::GetChild( const char * name ) {
	for( size_t i=0, n=m_Children.size(); i<n; ++i )
		if( !strcmp( m_Children[i]->GetName(), name ))
			return m_Children[i];
	return NULL;
}

void XmlNode::SetName( const char * name ) {
	m_Name = name;
}

static void Str( float f, std::string & s ) {
	char buf[64];
	sprintf_s( buf, "%0.4f", f );
	s = buf;
}

static void Str( int i, std::string & s ) {
	char buf[64];
	sprintf_s( buf, "%d", i );
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

void XmlNode::SetAttribute( const char * name, const char * s ) {
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
		if( m_Value.empty())
			tf.Write( "/>\n" );
		else
			tf.Write( ">%s</%s>\n", m_Value.c_str(), m_Name.c_str());
	} else {
		tf.Write( "<%s>\n", GetName());
		for( int i=0; i<GetNumChildren(); ++i )
			GetChild(i)->Save( tf );
		tf.Write( "</%s>\n", GetName());
	}
}

bool XmlNode::Load( TextFile & tf, const char * line ) {
	std::string str;
	if( !line ) {
		tf.ReadLine( str );
		line = &str[0];
	}
	std::string tag;
	int l, n;

	if( l = StrPatternMatch( line, " *<% ", &m_Name )) {
		std::string attr, val;
		for( ;; ) {
			if( n = StrPatternMatch( line+l, " *%=\"%\"", &attr, &val )) {
				l += n;
				SetAttribute( attr.c_str(), val.c_str() );
			} else if( StrPatternMatch( line+l, " *>" )) {
				break;
			} else if( StrPatternMatch( line+l, " */>" )) {
				return true;
			}
		}
	} else if( l = StrPatternMatch( line, " *<%>", &m_Name )) {

	} else {
		return false;
	}

	if( StrPatternMatch( line+l, "%</%>", &m_Value, &tag ))
		return true;

	for( ;; ) {
		tf.ReadLine( str );
		if( StrPatternMatch( str.c_str(), " *</%>", &tag ))
			return true;
		XmlNode * child = new XmlNode();
		if( !child->Load( tf, str.c_str() )) {
			delete child;
			return false;
		}
		AddChild( child );
	}
	return true;
}

bool XmlNode::Load( TextFile & tf ) {
	return Load( tf, NULL );
}

XmlNode * XmlNode::SerializeChild( const char * name ) {
	XmlNode * p = GetChild( name );
	if( p )
		return p;
	p = new XmlNode( name );
	AddChild( p );
	return p;
}

void KeyValuePtr::SetValue( const char * pc ) {
	if( str )
		*str = pc;
	else if( i )
		*i = atol( pc );
	else if( f )
		*f = (float)atof( pc );
}

std::string KeyValuePtr::GetValue() const {
	char pc[32];
	if( str )
		return *str;
	else if( i )
		sprintf_s( pc, "%d", *i );
	else if( f )
		sprintf_s( pc, "%0.6f", *f );
	return pc;
}

std::string KeyValuePtr::GetNameAsTag() const {
	std::string s( name );
	for( size_t i=0; i<s.size(); ++i )
		if( s[i] == ' ' )
			s[i] = '_';
	return s;
}

void KeyValuePtr::Serialize( XmlNode * pRoot ) {
	std::string tag = GetNameAsTag();
	XmlNode * node = pRoot->GetChild( tag.c_str() );
	if( node ) {
		SetValue( node->GetValue() );
	} else {
		node = new XmlNode( tag.c_str() );
		std::string s = GetValue();
		node->Set( s.c_str() );
		pRoot->AddChild( node );
	}
}

void Serialize( XmlNode * pRoot, const char * tag, std::vector<KeyValuePtr> & v ) {
	XmlNode * node = pRoot->GetChild( tag );
	if( !node ) {
		node = new XmlNode( tag );
		pRoot->AddChild( node );
	}
	for( size_t i=0; i<v.size(); ++i ) {
		v[i].Serialize( node );
	}
}

}