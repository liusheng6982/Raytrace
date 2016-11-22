#include "Common.h"

IntersectResult::IntersectResult() : triangle(NULL), material(NULL) {
}

Texture::Texture() : width(0), height(0) {}

void Texture::Init( NanoCore::Image::Ptr pImage ) {
	width = pImage->GetWidth();
	height = pImage->GetHeight();

	mips.push_back( pImage );
	for( int w=width/2, h=height/2; w>1 && h>1; w /= 2, h /= 2 ) {
		NanoCore::Image::Ptr img( new NanoCore::Image( w, h, pImage->GetBpp()) );
		img->Stretch( *mips.back() );
		mips.push_back( img );
	}
}

void Texture::GetTexel( float2 uv, float4 & pix ) const {
	int t[4] = {0};
	GetTexel( uv, t );
	pix.x = t[0]/255.0f;
	pix.y = t[1]/255.0f;
	pix.z = t[2]/255.0f;
	pix.w = t[3]/255.0f;
}

void Texture::GetTexel( float2 uv, int pix[4] ) const {
	if( IsEmpty()) {
		pix[0] = pix[1] = pix[2] = pix[3] = 0;
		return;
	}
	mips[0]->GetPixel( uv.x, uv.y, pix );
}

float3 Texture::GetTexel( float2 uv ) const {
	float4 res;
	GetTexel( uv, res );
	return float3( res.x, res.y, res.z );
}

Material::Material() : opacity(1.0f), Ns(0.0f) {
	Kd = float3(1.0f);
	Ke = float3(0.0f);
}