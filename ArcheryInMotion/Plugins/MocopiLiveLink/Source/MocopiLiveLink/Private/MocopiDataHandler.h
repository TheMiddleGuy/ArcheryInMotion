////
// Copyright (C) 2026 Sony Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
////

#pragma once

#include <vector> 
#include "MocopiMotionFormat.h"

struct MocopiBoneData
{
  const char* jointName;

  int         parentIndex;

  double      translate[3];
  double      rotate[4];

  double	  defaultTranslate[3];
  double	  defaultRotate[4];
};

struct MocopiFrameMetaData
{
  int frameId;

  float timeStamp;

  double utcTime;

  int numBones;

  unsigned long long senderIp;
  int senderPort;

  std::vector<int> jointIds;
  std::vector<int> parentJointIds;

  struct Timecode {
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t frame;
	uint8_t frameRate;
	bool dropFrame;

	bool isValid() 
	{
	  return (hour > 0 || min > 0 || sec > 0 || frame > 0);
	}
  };

  Timecode timecode;
};

class MocopiDataHandler : public MocopiMotionFormat {

public:

	MocopiDataHandler();

	void ProcessSkeletonDefinitionData(std::byte* dataBuffer, int bufferSize);

	void ProcessFrameData(std::byte* dataBuffer, int bufferSize);

	bool IsSkeletonDefinition(std::byte* bytes, int bufferSize);
	bool IsFrameData(std::byte* dataBuffer, int bufferSize);

	const MocopiBoneData& GetBoneInfoByIndex(int boneIndex);

	int GetNumBones();

	const std::vector<MocopiBoneData>& GetFrameBoneDataArray() const;

	const MocopiFrameMetaData GetFrameMetaData();

private:

	// Unvariable joint number at this point.
	const int SKELETON_NUM_BONES = 27;

	// For scaling translation values
	const double BONE_SCALE = 100.0;

	std::vector<MocopiBoneData>	mBoneDataArray;

	MocopiFrameMetaData mFrameMetaData;

	void InitializeSkeletonChannels();

};
