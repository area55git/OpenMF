#ifndef FORMAT_PARSERS_SCENE2_BIN_H
#define FORMAT_PARSERS_SCENE2_BIN_H

#include <cstring>

#include <base_parser.hpp>

#include <stdio.h>
#include <iostream>

namespace MFFormat
{

class DataFormatScene2BIN: public DataFormat
{
public:
    typedef enum {
        // top Headers
       HEADER_MISSION = 0x4c53,
       HEADER_META = 0x0001,
       HEADER_UNK_FILE = 0xAFFF,
       HEADER_UNK_FILE2 = 0x3200,
       HEADER_FOV = 0x3010,
       HEADER_VIEW_DISTANCE = 0x3011,
       HEADER_CLIPPING_PLANES = 0x3211,
       HEADER_WORLD = 0x4000,
       HEADER_ENTITIES = 0xAE20,
       HEADER_INIT = 0xAE50,
        // WORLD subHeader
       HEADER_OBJECT = 0x4010
    } HeaderType;

    typedef enum {
        OBJECT_TYPE = 0x4011,
        OBJECT_POSITION = 0x0020,
        OBJECT_ROTATION = 0x0022,
        OBJECT_POSITION_2 = 0x002C,
        OBJECT_SCALE = 0x002D,
        OBJECT_PARENT = 0x4020,
        OBJECT_NAME = 0x0010, 
        OBJECT_MODEL = 0x2012,
        OBJECT_LIGHT_MAIN = 0x4040,
        OBJECT_LIGHT_TYPE = 0x4041,
        OBJECT_LIGHT_COLOUR = 0x0026,
        OBJECT_LIGHT_POWER = 0x4042,
        OBJECT_LIGHT_UNK_1 = 0x4043,
        OBJECT_LIGHT_RANGE = 0x4044,
        OBJECT_LIGHT_FLAGS = 0x4045,
        OBJECT_LIGHT_SECTOR= 0x4046,
        OBJECT_LIGHT_MAP= 0x40A0
    } ObjectProperty;

    typedef enum {
        OBJECT_TYPE_LIGHT = 0x02,
        OBJECT_TYPE_CAMERA = 0x03,
        OBJECT_TYPE_SOUND = 0x04,
        OBJECT_TYPE_MODEL = 0x09,
        OBJECT_TYPE_OCCLUDER = 0x0C,
        OBJECT_TYPE_SECTOR = 0x99,
        OBJECT_TYPE_SCRIPT = 0x9B
    } ObjectType;

    typedef enum {
        LIGHT_TYPE_POINT = 0x01,
        LIGHT_TYPE_DIRECTIONAL = 0x03,
        LIGHT_TYPE_AMBIENT = 0x04,
        LIGHT_TYPE_FOG = 0x05,
        LIGHT_TYPE_POINT_AMBIENT = 0x06,
        LIGHT_TYPE_LAYERED_FOG = 0x08,
    } LightType;

    #pragma pack(push, 1)
    typedef struct
    {
        uint16_t mType;
        uint32_t mSize;
    } Header;
    #pragma pack(pop)

    typedef struct _Object
    {
        uint32_t mType;
        Vec3 mPos;
        Quat mRot;
        Vec3 mPos2; // precomputed final world transform position
        Vec3 mScale;
        std::string mName;
        std::string mModelName;
        std::string mParentName;

        // Light properties
        uint32_t mLightType;
        Vec3 mLightColour;
        int32_t mLightFlags;
        float mLightPower;
        float mLightUnk0;
        float mLightUnk1;
        float mLightNear;
        float mLightFar;
        char mLightSectors[5000];
    } Object;

    virtual bool load(std::ifstream &srcFile);
    
    inline size_t getNumObjects()                               { return mObjects.size(); }
    inline Object* getObject(std::string name)                  { return &mObjects.at(name); }
    inline std::unordered_map<std::string, Object> getObjects() { return mObjects; }
    inline float getFov()                                       { return mFov; }
    inline void setFov(float value)                             { mFov = value; }
    inline float getViewDistance()                              { return mViewDistance; }
    inline void setViewDistance(float value)                    { mViewDistance = value; }
    inline Vec2  getClippingPlanes()                            { return mClippingPlanes; }
    inline void  setClippingPlanes(Vec2 value)                  { mClippingPlanes = value; }

private:
    void readHeader(std::ifstream &srcFile, Header* header, uint32_t offset);
    void readObject(std::ifstream &srcFile, Header* header, Object* object, uint32_t offset);
    void readLight (std::ifstream &srcFile, Header* header, Object* object);
    
    std::unordered_map<std::string, Object> mObjects;
    float mFov;
    float mViewDistance;
    Vec2  mClippingPlanes;
};

bool DataFormatScene2BIN::load(std::ifstream &srcFile)
{
    Header newHeader = {};
    read(srcFile, &newHeader);
    uint32_t position = 6;

    while(position + 6 < newHeader.mSize)
    {
        srcFile.seekg(position, srcFile.beg);
        Header nextHeader = {};
        read(srcFile, &nextHeader);
        readHeader(srcFile, &nextHeader, position + 6);
        position += nextHeader.mSize;
    }

    return true;
}

void DataFormatScene2BIN::readHeader(std::ifstream &srcFile, Header* header, uint32_t offset)
{
    switch(header->mType)
    {
        case HEADER_WORLD:
        {
            uint32_t position = offset;
            while(position + 6 < offset + header->mSize)
            {
                Header nextHeader = {};
                srcFile.seekg(position, srcFile.beg);
                read(srcFile, &nextHeader);
                readHeader(srcFile, &nextHeader, position + 6);
                position += nextHeader.mSize;
            }
        }
        break;

        case HEADER_VIEW_DISTANCE:
        {
            read(srcFile, &mViewDistance);
        } 
        break;

        case HEADER_CLIPPING_PLANES:
        {
            read(srcFile, &mClippingPlanes);
        }
        break;

        case HEADER_FOV:
        {
            read(srcFile, &mFov);
        } 
        break;

        case HEADER_OBJECT:
        {
            uint32_t position = offset;
            Object newObject = {};
            while(position + 6 < offset + header->mSize)
            {
                Header nextHeader = {};
                srcFile.seekg(position, srcFile.beg);
                read(srcFile, &nextHeader);
                readObject(srcFile, &nextHeader, &newObject, position + 6);
                position += nextHeader.mSize;
            }

            mObjects.insert(make_pair(newObject.mName, newObject));
        } 
        break;
    }
}

void DataFormatScene2BIN::readObject(std::ifstream &srcFile, Header* header, Object* object, uint32_t offset)
{
    switch(header->mType)
    {
        case OBJECT_TYPE:
        {
            read(srcFile, &object->mType);
        }
        break;

        case OBJECT_NAME:
        {
            char *name = reinterpret_cast<char*>(malloc(header->mSize + 1));
            read(srcFile, name, header->mSize - 1);
            name[header->mSize] = '\0';

            object->mName = std::string(name);
            free(name);
        }
        break;

        case OBJECT_MODEL:
        {
            char *modelName = reinterpret_cast<char*>(malloc(header->mSize + 1));
            read(srcFile, modelName, header->mSize);
            modelName[strlen(modelName) - 4] = '\0';
            sprintf(modelName, "%s.4ds", modelName);
            modelName[header->mSize] = '\0';

            object->mModelName = std::string(modelName);
            free(modelName);
        }
        break;

        case OBJECT_POSITION:
        {
            Vec3 newPosition = {};
            read(srcFile, &newPosition);
            object->mPos = newPosition;
        } 
        break;

        case OBJECT_ROTATION:
        {
            Quat newRotation = {};
            read(srcFile, &newRotation);
            newRotation.fromMafia();
            object->mRot = newRotation;
        } 
        break;

        case OBJECT_POSITION_2:
        {
            Vec3 newPosition = {};
            read(srcFile, &newPosition);
            object->mPos2 = newPosition;
        } 
        break;

        case OBJECT_SCALE:
        {
            Vec3 newScale = {};
            read(srcFile, &newScale);
            object->mScale = newScale;
        } 
        break;

        case OBJECT_LIGHT_MAIN:
        {
            uint32_t position = offset;
            while (position + 6 < offset + header->mSize)
            {
                Header lightHeader = {};
                read(srcFile, &lightHeader);
                readLight(srcFile, &lightHeader, object);
                position += lightHeader.mSize;
            }
        }
        break;

        case OBJECT_PARENT:
        {
            Header parentHeader = {};
            read(srcFile, &parentHeader);
            Object parentObject = {};
            readObject(srcFile, &parentHeader, &parentObject, offset + 6);

            object->mParentName = parentObject.mName;
        }
        break;
        case OBJECT_LIGHT_MAP:
        {
            uint8_t numberOfLightmapLevels;
            read(srcFile, &numberOfLightmapLevels);
            // According to docs, unk word should be 2015/2016
            uint16_t unknownWord;
            read(srcFile, &unknownWord);
            // 01 00 00 00
            uint32_t unknownDword;
            read(srcFile, &unknownDword);

            float unkFloatA;
            read(srcFile, &unkFloatA);

            float unkFloatB;
            read(srcFile, &unkFloatB);

            std::cout << "Obj. lightmap starting offset: " <<  srcFile.tellg() << "\n";
            std::cout << "\tSection1:" << "\n";
            std::cout << "\tLightmap levels:\t" << (int) numberOfLightmapLevels << "\n";
            std::cout << "\tFloat A:\t" << unkFloatA<< "\n";
            std::cout << "\tFloat B:\t" << unkFloatB<< "\n";

            uint8_t numberOfLevels;
            read(srcFile, &numberOfLevels);
            std::cout << "\t" <<  (int) numberOfLevels << " levels" << "\n";
        
            uint16_t numberOfVertices;
            read(srcFile, &numberOfVertices);

            uint16_t numberOfLBMP;
            read(srcFile, &numberOfLBMP);

            uint16_t typeOfFollowing;
            read(srcFile, &typeOfFollowing);


            std::cout << "\t" << "Section2:\n";
            std::cout << "\t\t" << "noVert:\t"<<numberOfVertices<<"\n";
            std::cout << "\t\t" << "noLBMP:\t"<<numberOfLBMP<<"\n";
            std::cout << "\t\t" << "typeFollows:\t"<<typeOfFollowing<<"\n";

            
        }
        break;

    }
}

void DataFormatScene2BIN::readLight(std::ifstream &srcFile, Header* header, Object* object)
{
    switch(header->mType)
    {
        case OBJECT_LIGHT_TYPE:
        {
            read(srcFile, &object->mLightType);
        }
        break;
        
        case OBJECT_LIGHT_COLOUR:
        {
            read(srcFile, &object->mLightColour);
        }
        break;
        
        case OBJECT_LIGHT_POWER:
        {
            read(srcFile, &object->mLightPower);
        }
        break;
        
        case OBJECT_LIGHT_RANGE:
        {
            read(srcFile, &object->mLightNear);
            read(srcFile, &object->mLightFar);
        }
        break;
        
        case OBJECT_LIGHT_SECTOR:
        {
            //read(srcFile, object->mLightSectors, header->mSize);
        }
        break;
        
        case OBJECT_LIGHT_FLAGS:
        {
            read(srcFile, &object->mLightFlags);
        }
        break;

        case OBJECT_LIGHT_UNK_1:
        {
            read(srcFile, &object->mLightUnk0);
            read(srcFile, &object->mLightUnk1);
        }
        break;
    }
}

}

#endif
