// smol-v - tests code - public domain - https://github.com/aras-p/smol-v
// authored on 2016 by Aras Pranckevicius
// no warranty implied; use at your own risk

#define _CRT_SECURE_NO_WARNINGS // yes MSVC, I want to use fopen
#include "../source/smolv.h"
#include "external/lz4/lz4.h"
#include "external/lz4/lz4hc.h"
#include "external/zstd/zstd.h"
#include "external/glslang/SPIRV/SPVRemapper.h"
#include <stdio.h>
#include <map>
#include <string>

typedef std::vector<uint8_t> ByteArray;


static void ReadFile(const char* fileName, ByteArray& output)
{
	FILE* f = fopen(fileName, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		size_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		size_t pos = output.size();
		output.resize(pos + size);
		fread(output.data() + pos, size, 1, f);
		fclose(f);
	}
}

static void RemapSPIRV(const void* data, size_t size, ByteArray& output)
{
	const uint32_t* dataI = (const uint32_t*)data;
	const size_t sizeI = size/4;
	std::vector<uint32_t> buf(dataI, dataI+sizeI);
	
	spv::spirvbin_t remapper;
	remapper.remap(buf, remapper.ALL_BUT_STRIP);
	output.insert(output.end(), (const uint8_t*)buf.data(), ((const uint8_t*)buf.data()) + buf.size()*4);
}

static size_t CompressLZ4HC(const void* data, size_t size, int level = 0)
{
	if (size == 0)
		return 0;
	int bufferSize = LZ4_compressBound((int)size);
	char* buffer = new char[bufferSize];
	size_t resSize = LZ4_compress_HC ((const char*)data, buffer, (int)size, bufferSize, level);
	delete[] buffer;
	return resSize;
}

static size_t CompressZstd(const void* data, size_t size, int level = 0)
{
	if (size == 0)
		return 0;
	size_t bufferSize = ZSTD_compressBound(size);
	char* buffer = new char[bufferSize];
	size_t resSize = ZSTD_compress(buffer, bufferSize, data, size, level);
	delete[] buffer;
	return resSize;
}


int main()
{
	smolv::Stats* stats = smolv::StatsCreate();

	// files we're testing on
	const char* kFiles[] =
	{
		// Shaders produced by Unity's pipeline (HLSL -> DX11 bytecode -> HLSLcc -> glslang); vertex shaders
		"tests/spirv-dumps/s0-0001-32333750.spirv",
		"tests/spirv-dumps/s0-0002-ca3af858.spirv",
		"tests/spirv-dumps/s0-0003-6ccb7b5e.spirv",
		"tests/spirv-dumps/s0-0004-9218583a.spirv",
		"tests/spirv-dumps/s0-0004-d2ba7e35.spirv",
		"tests/spirv-dumps/s0-0005-1eb77240.spirv",
		"tests/spirv-dumps/s0-0006-c36e44b3.spirv",
		"tests/spirv-dumps/s0-0007-3454fc86.spirv",
		"tests/spirv-dumps/s0-0007-ca8748eb.spirv",
		"tests/spirv-dumps/s0-0008-ecc6f669.spirv",
		"tests/spirv-dumps/s0-0009-e4ff870b.spirv",
		"tests/spirv-dumps/s0-0010-9379a08b.spirv",
		"tests/spirv-dumps/s0-0011-19016092.spirv",
		"tests/spirv-dumps/s0-0011-fe5ada8b.spirv",
		"tests/spirv-dumps/s0-0012-fba002b2.spirv",
		"tests/spirv-dumps/s0-0012-fc99d1e7.spirv",
		"tests/spirv-dumps/s0-0013-2190e4b3.spirv",
		"tests/spirv-dumps/s0-0013-aaccb753.spirv",
		"tests/spirv-dumps/s0-0013-b945d8f9.spirv",
		"tests/spirv-dumps/s0-0014-3ba60738.spirv",
		"tests/spirv-dumps/s0-0014-42341ba0.spirv",
		"tests/spirv-dumps/s0-0015-1b4af3ab.spirv",
		"tests/spirv-dumps/s0-0016-cc22f312.spirv",
		"tests/spirv-dumps/s0-0017-9d156c5b.spirv",
		"tests/spirv-dumps/s0-0018-267005da.spirv",
		"tests/spirv-dumps/s0-0019-68bab1cd.spirv",
		"tests/spirv-dumps/s0-0020-cb89e824.spirv",
		"tests/spirv-dumps/s0-0021-774ade5f.spirv",
		"tests/spirv-dumps/s0-0023-e226aa8a.spirv",
		"tests/spirv-dumps/s0-0024-9b5e5139.spirv",
		"tests/spirv-dumps/s0-0025-e1567d6d.spirv",
		"tests/spirv-dumps/s0-0026-717968d2.spirv",
		"tests/spirv-dumps/s0-0027-223d615c.spirv",
		"tests/spirv-dumps/s0-0028-290869cd.spirv",
		"tests/spirv-dumps/s0-0029-fc9c1174.spirv",
		"tests/spirv-dumps/s0-0031-1a0d9226.spirv",
		"tests/spirv-dumps/s0-0032-9f4fab1d.spirv",
		"tests/spirv-dumps/s0-0034-5c3d45dc.spirv",
		"tests/spirv-dumps/s0-0045-f2078956.spirv",
		"tests/spirv-dumps/s0-0046-0b926d9c.spirv",
		// Shaders produced by Unity's pipeline (HLSL -> DX11 bytecode -> HLSLcc -> glslang); fragment shaders
		"tests/spirv-dumps/s1-0000-5ca04fe4.spirv",
		"tests/spirv-dumps/s1-0000-cf9fe2e0.spirv",
		"tests/spirv-dumps/s1-0001-04d9d27b.spirv",
		"tests/spirv-dumps/s1-0001-aa1ecaf0.spirv",
		"tests/spirv-dumps/s1-0002-8d2ed6da.spirv",
		"tests/spirv-dumps/s1-0003-c54216ae.spirv",
		"tests/spirv-dumps/s1-0004-ac0f5549.spirv",
		"tests/spirv-dumps/s1-0005-93ebd823.spirv",
		"tests/spirv-dumps/s1-0006-85d79507.spirv",
		"tests/spirv-dumps/s1-0007-aff64c99.spirv",
		"tests/spirv-dumps/s1-0008-6e421249.spirv",
		"tests/spirv-dumps/s1-0009-0c858280.spirv",
		"tests/spirv-dumps/s1-0010-1b50ab90.spirv",
		"tests/spirv-dumps/s1-0011-2fed16ab.spirv",
		"tests/spirv-dumps/s1-0011-f3d46288.spirv",
		"tests/spirv-dumps/s1-0012-21b778d5.spirv",
		"tests/spirv-dumps/s1-0012-5428b42d.spirv",
		"tests/spirv-dumps/s1-0013-241e9fc8.spirv",
		"tests/spirv-dumps/s1-0013-35edd084.spirv",
		"tests/spirv-dumps/s1-0014-2ea8dc83.spirv",
		"tests/spirv-dumps/s1-0014-5c2d2a73.spirv",
		"tests/spirv-dumps/s1-0015-3b3a60bf.spirv",
		"tests/spirv-dumps/s1-0016-3c30e5e7.spirv",
		"tests/spirv-dumps/s1-0016-40492bcb.spirv",
		"tests/spirv-dumps/s1-0017-6c18345b.spirv",
		"tests/spirv-dumps/s1-0017-884cc79d.spirv",
		"tests/spirv-dumps/s1-0018-319798ba.spirv",
		"tests/spirv-dumps/s1-0019-1e7cb4ff.spirv",
		"tests/spirv-dumps/s1-0020-7363f5c5.spirv",
		"tests/spirv-dumps/s1-0021-e914f581.spirv",
		"tests/spirv-dumps/s1-0022-30eff697.spirv",
		"tests/spirv-dumps/s1-0024-0ce8d0e7.spirv",
		"tests/spirv-dumps/s1-0025-3c7f4035.spirv",
		"tests/spirv-dumps/s1-0026-1db01998.spirv",
		"tests/spirv-dumps/s1-0027-51fa0f15.spirv",
		"tests/spirv-dumps/s1-0028-84042ffc.spirv",
		"tests/spirv-dumps/s1-0029-8b7741ae.spirv",
		"tests/spirv-dumps/s1-0030-b919e80f.spirv",
		"tests/spirv-dumps/s1-0032-c069697f.spirv",
		"tests/spirv-dumps/s1-0033-0b804090.spirv",
		"tests/spirv-dumps/s1-0034-bad8bbff.spirv",
		"tests/spirv-dumps/s1-0035-e2a55d77.spirv",
		"tests/spirv-dumps/s1-0036-6bbcc1ac.spirv",
		"tests/spirv-dumps/s1-0037-e09cedbe.spirv",
		"tests/spirv-dumps/s1-0038-e85d5917.spirv",
		"tests/spirv-dumps/s1-0039-09bb7e61.spirv",
		"tests/spirv-dumps/s1-0041-834a25b0.spirv",
		"tests/spirv-dumps/s1-0043-026e1b4a.spirv",
		"tests/spirv-dumps/s1-0045-f49f5967.spirv",
		"tests/spirv-dumps/s1-0047-9c22101b.spirv",
		"tests/spirv-dumps/s1-0049-6dd06f97.spirv",
		"tests/spirv-dumps/s1-0052-d2e4133a.spirv",
		"tests/spirv-dumps/s1-0053-511daec1.spirv",
		"tests/spirv-dumps/s1-0054-b2b7e6c0.spirv",
		"tests/spirv-dumps/s1-0055-66f03021.spirv",
		"tests/spirv-dumps/s1-0056-89c781a9.spirv",
		"tests/spirv-dumps/s1-0057-87e01eae.spirv",
		"tests/spirv-dumps/s1-0062-e52c10e6.spirv",
		"tests/spirv-dumps/s1-0063-70e7171c.spirv",
		"tests/spirv-dumps/s1-0070-7595e017.spirv",
		"tests/spirv-dumps/s1-0074-e4935128.spirv",
		"tests/spirv-dumps/s1-0084-ffb8278d.spirv",
		// Shaders produced by Unity's pipeline (HLSL -> DX11 bytecode -> HLSLcc -> glslang); hull shaders
		"tests/spirv-dumps/s2-0004-76b9ef38.spirv",
		"tests/spirv-dumps/s2-0006-655ac983.spirv",
		"tests/spirv-dumps/s2-0019-3ddaf08d.spirv",
		"tests/spirv-dumps/s2-0031-412ed89d.spirv",
		// Shaders produced by Unity's pipeline (HLSL -> DX11 bytecode -> HLSLcc -> glslang); domain shaders
		"tests/spirv-dumps/s3-0008-09cef3e4.spirv",
		"tests/spirv-dumps/s3-0019-4c006911.spirv",
		"tests/spirv-dumps/s3-0022-f40e2e1e.spirv",
		"tests/spirv-dumps/s3-0028-e081a509.spirv",
		"tests/spirv-dumps/s3-0037-18f71ada.spirv",
		// Shaders produced by Unity's pipeline (HLSL -> DX11 bytecode -> HLSLcc -> glslang); geometry shaders
		"tests/spirv-dumps/s4-0004-6ec33743.spirv",
		"tests/spirv-dumps/s4-0006-a5e06270.spirv",
	};

	// all test data lumped together, to check how well it compresses as a whole block
	ByteArray spirvAll;
	ByteArray spirvRemapAll;
	ByteArray smolvAll;

	// go over all test files
	int errorCount = 0;
	for (size_t i = 0; i < sizeof(kFiles)/sizeof(kFiles[0]); ++i)
	{
		// Read
		printf("Reading %s\n", kFiles[i]);
		ByteArray spirv;
		ReadFile(kFiles[i], spirv);
		if (spirv.empty())
		{
			printf("ERROR: failed to read %s\n", kFiles[i]);
			++errorCount;
			break;
		}

		// Basic SPIR-V input stats
		if (!smolv::StatsCalculate(stats, spirv.data(), spirv.size()))
		{
			printf("ERROR: failed to calc instruction stats (invalid SPIR-V?) %s\n", kFiles[i]);
			++errorCount;
			break;
		}

		// Encode to SMOL-V
		ByteArray smolv;
		if (!smolv::Encode(spirv.data(), spirv.size(), smolv))
		{
			printf("ERROR: failed to encode (invalid invalid SPIR-V?) %s\n", kFiles[i]);
			++errorCount;
			break;
		}

		// Decode back to SPIR-V
		size_t spirvDecodedSize = smolv::GetDecodedBufferSize(smolv.data(), smolv.size());
		ByteArray spirvDecoded;
		spirvDecoded.resize(spirvDecodedSize);
		if (!smolv::Decode(smolv.data(), smolv.size(), spirvDecoded.data(), spirvDecodedSize))
		{
			printf("ERROR: failed to decode back (bug?) %s\n", kFiles[i]);
			++errorCount;
			break;
		}

		// Check that it decoded 100% the same
		if (spirv != spirvDecoded)
		{
			printf("ERROR: did not encode+decode properly (bug?) %s\n", kFiles[i]);
			const uint8_t* spirvPtr = spirv.data();
			const uint8_t* spirvPtrEnd = spirvPtr + spirv.size();
			const uint8_t* spirvDecodedPtr = spirvDecoded.data();
			const uint8_t* spirvDecodedEnd = spirvDecodedPtr + spirvDecoded.size();
			int idx = 0;
			while (spirvPtr < spirvPtrEnd && spirvDecodedPtr < spirvDecodedEnd)
			{
				if (*spirvPtr != *spirvDecodedPtr)
				{
					printf("  byte #%04i %02x -> %02x\n", idx, *spirvPtr, *spirvDecodedPtr);
				}
				++spirvPtr;
				++spirvDecodedPtr;
				++idx;
			}
			
			++errorCount;
			break;
		}

		// SMOL encoding stats
		if (!smolv::StatsCalculateSmol(stats, smolv.data(), smolv.size()))
		{
			printf("ERROR: failed to calc SMOLV instruction stats (bug?) %s\n", kFiles[i]);
			++errorCount;
			break;
		}

		// Append original and SMOLV code to the whole blob
		spirvAll.insert(spirvAll.end(), spirv.begin(), spirv.end());
		smolvAll.insert(smolvAll.end(), smolv.begin(), smolv.end());
		RemapSPIRV(spirv.data(), spirv.size(), spirvRemapAll);
	}

	// Compress various ways (as a whole blob) and print sizes
	printf("Compressing...\n");
	typedef std::map<std::string, size_t> CompressorSizeMap;
	CompressorSizeMap sizes;

	;;sizes["0 Remap"] = spirvRemapAll.size();
	sizes["0 SMOL-V"] = smolvAll.size();

	;;sizes["1    LZ4HC"] = CompressLZ4HC(spirvAll.data(), spirvAll.size());
	;;sizes["1 re+LZ4HC"] = CompressLZ4HC(spirvRemapAll.data(), spirvRemapAll.size());
	sizes["1 sm+LZ4HC"] = CompressLZ4HC(smolvAll.data(), smolvAll.size());

	;;sizes["2    Zstd"] = CompressZstd(spirvAll.data(), spirvAll.size());
	;;sizes["2 re+Zstd"] = CompressZstd(spirvRemapAll.data(), spirvRemapAll.size());
	sizes["2 sm+Zstd"] = CompressZstd(smolvAll.data(), smolvAll.size());

	;;sizes["3    Zstd20"] = CompressZstd(spirvAll.data(), spirvAll.size(), 20);
	;;sizes["3 re+Zstd20"] = CompressZstd(spirvRemapAll.data(), spirvRemapAll.size(), 20);
	sizes["3 sm+Zstd20"] = CompressZstd(smolvAll.data(), smolvAll.size(), 20);
	
	smolv::StatsPrint(stats);
	smolv::StatsDelete(stats);
	
	printf("Compression: original size %.1fKB\n", spirvAll.size()/1024.0f);
	for (CompressorSizeMap::const_iterator it = sizes.begin(), itEnd = sizes.end(); it != itEnd; ++it)
	{
		printf("%-13s %6.1fKB %5.1f%%\n", it->first.c_str(), it->second/1024.0f, (float)it->second/(float)(spirvAll.size())*100.0f);
	}
	

	if (errorCount != 0)
	{
		printf("Got ERRORS: %i\n", errorCount);
		return 1;
	}
	return 0;
}
