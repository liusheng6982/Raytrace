#include "Common.h"



IntersectResult::IntersectResult() : triangle(NULL), material(NULL), flags(0) {
}

void IntersectResult::SetUV( float2 _uv ) {
	uv = _uv;
	flags |= eUV;
}

void IntersectResult::SetInterpolatedNormal( float3 n ) {
	interpolatedNormal = n;
	flags |= eNormal;
}

void IntersectResult::SetTangentSpace( float3 t, float3 b ) {
	tangent = t;
	bitangent = b;
	flags |= eTangentSpace;
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

void Texture::GetTexelFiltered( const float2 & uv, float best_resolution, float4 & pix ) const {
	int mip = 0;

	assert( best_resolution >= 1.0f );

	int best = (int)ncFloor( best_resolution );
	int mipRes = Max( width, height );
	int numMips = mips.size();

	for( mip = 0; mipRes/2 > best && mip+1 < numMips; ++mip, mipRes /= 2 );

	int px[4];

	if( mip+1 == numMips ) {
		mips.back()->GetPixelBilinear( uv.x, uv.y, px );
	} else {
		int p0[4], p1[4];
		mips[mip]->GetPixelBilinear( uv.x, uv.y, p0 );
		mips[mip+1]->GetPixelBilinear( uv.x, uv.y, p1 );

		int trilerp1k = (best - mips[mip]->GetWidth()) * 1024 / (mips[mip]->GetWidth() - mips[mip+1]->GetWidth());

		NanoCore::Image::LerpColor( px, p0, p1, trilerp1k, mips[0]->GetBpp() );
	}
	pix.x = px[0] * (1.0f/255.0f);
	pix.y = px[1] * (1.0f/255.0f);
	pix.z = px[2] * (1.0f/255.0f);
	pix.w = px[3] * (1.0f/255.0f);
}

Material::Material() : opacity(1.0f), Ns(0.0f) {
	Kd = float3(1.0f);
	Ke = float3(0.0f);
}