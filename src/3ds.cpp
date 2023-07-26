
//***********************************************************************//
//                                                                       //
//        - "Talk to me like I'm a 3 year old!" Programming Lessons -    //
//                                                                       //
//        $Author:        DigiBen        digiben@gametutorials.com       //
//                                                                       //
//        $Program:        3DS Loader                                    //
//                                                                       //
//        $Description:    Demonstrates how to load a .3ds file format   //
//                                                                       //
//        $Date:            10/6/01                                      //
//                                                                       //
//***********************************************************************//

// Jan 2003 - Changes by Danny Chapman to (a) make it build under
// linux/Solaris and (b) fix some potential write-beyond-stack
// problems I was experiencing with reading certain 3DS files - in
// particular reading 117 bytes into a 4byte stack variable...

#include "3ds.h"
#include <stdio.h>
#include <math.h>
#if defined(__ppc__)
#include "sss_ppc.h"
#endif
#include "log_trace.h"
#include "sss_assert.h"

void show_chunk(tChunk * pChunk)
{
  TRACE("Chunk %p: id = %04X, len = %08x, read = %d\n",
        pChunk, pChunk->ID, pChunk->length, pChunk->bytesRead);
}

#define BUF_SIZE 50000    
static union Buf_union 
{
  int buf[BUF_SIZE];
  float fbuf[BUF_SIZE];
  unsigned char cbuf[4*BUF_SIZE];
  unsigned char val_uchar;
  char val_char;
  short val_short;
  unsigned short val_ushort;
  unsigned int val_uint;
  int val_int;
} buf_union;

// This file handles all of the code needed to load a .3DS file.
// Basically, how it works is, you load a chunk, then you check
// the chunk ID.  Depending on the chunk ID, you load the information
// that is stored in that chunk.  If you do not want to read that information,
// you read past it.  You know how many bytes to read past the chunk because
// every chunk stores the length in bytes of that chunk.

///////////////////////////////// CLOAD3DS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This constructor initializes the tChunk data
/////
///////////////////////////////// CLOAD3DS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

CLoad3DS::CLoad3DS()
{
  TRACE_METHOD_ONLY(2);
  
  m_CurrentChunk = new tChunk;         // Initialize and allocate our current chunk
  m_TempChunk = new tChunk;            // Initialize and allocate a temporary chunk
}

///////////////////////////////// IMPORT 3DS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This is called by the client to open the .3ds file, read it, then clean up
/////
///////////////////////////////// IMPORT 3DS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

bool CLoad3DS::Import3DS(t3DModel *pModel, const char *strFileName)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("pModel = %p, file = %s\n", pModel, strFileName);}
  
  // Open the 3DS file
  m_FilePointer = fopen(strFileName, "rb");
  
  // Make sure we have a valid file pointer (we found the file)
  if(!m_FilePointer) 
  {
    TRACE("Unable to find the file: %s!\n", strFileName);
    return false;
  }
  
  // Once we have the file open, we need to read the very first data chunk
  // to see if it's a 3DS file.  That way we don't read an invalid file.
  // If it is a 3DS file, then the first chunk ID will be equal to PRIMARY (some hex num)
  
  // Read the first chuck of the file to see if it's a 3DS file
  ReadChunk(m_CurrentChunk);
  
  // Make sure this is a 3DS file
  if (m_CurrentChunk->ID != PRIMARY)
  {
    TRACE("Unable to load PRIMARY chuck from file: %s!\n", strFileName);
    return false;
  }
  
  // Now we actually start reading in the data.  ProcessNextChunk() is recursive
  
  // Begin loading objects, by calling this recursive function
  ProcessNextChunk(pModel, m_CurrentChunk);
  
  // After we have read the whole 3DS file, we want to calculate our own vertex normals.
  ComputeNormals(pModel);
  
  // Clean up after everything
  CleanUp();
  
  return true;
}

///////////////////////////////// CLEAN UP \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function cleans up our allocated memory and closes the file
/////
///////////////////////////////// CLEAN UP \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void CLoad3DS::CleanUp()
{
  TRACE_METHOD_ONLY(2);
  fclose(m_FilePointer);                        // Close the current file pointer
  delete m_CurrentChunk;                        // Free the current chunk
  delete m_TempChunk;                            // Free our temporary chunk
}


///////////////////////////////// PROCESS NEXT CHUNK\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function reads the main sections of the .3DS file, 
/////    then dives deeper with recursion
/////
///////////////////////////////// PROCESS NEXT CHUNK\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void CLoad3DS::ProcessNextChunk(t3DModel *pModel, tChunk *pPreviousChunk)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("pModel = %p, tChunk = %p\n", pModel, pPreviousChunk);}
  t3DObject newObject;             // This is used to add to our object list
  tMaterialInfo newTexture;  // This is used to add to our material list
  unsigned int version = 0;        // This will hold the file version
  unsigned int num_read;
  unsigned int num_to_read;
  vector<int> buffer(BUF_SIZE); // This is used to read past unwanted data

  m_CurrentChunk = new tChunk;         // Allocate a new chunk                
  
  // Below we check our chunk ID each time we read a new chunk.  Then, if
  // we want to extract the information from that chunk, we do so.
  // If we don't want a chunk, we just read past it.  
  
  // Continue to read the sub chunks until we have reached the length.
  // After we read ANYTHING we add the bytes read to the chunk and then check
  // check against the length.
  while (pPreviousChunk->bytesRead < pPreviousChunk->length)
  {
    // Read next Chunk
    ReadChunk(m_CurrentChunk);
    
    // Check the chunk ID
    switch (m_CurrentChunk->ID)
    {
    case VERSION:                     // This holds the version of the file
      
      // This chunk has an unsigned short that holds the file version.
      // Since there might be new additions to the 3DS file format in 4.0,
      // we give a warning to that problem.
      
      num_to_read = m_CurrentChunk->length - m_CurrentChunk->bytesRead;
      assert1(num_to_read <= BUF_SIZE);
      TRACE_FILE_IF(2)
        TRACE("Reading file VERSION: %d bytes\n", num_to_read);
      // Read the file version and add the bytes read to our bytesRead variable
      num_read = fread(&buf_union, 1, num_to_read, m_FilePointer);
      assert1(num_read == num_to_read);

#if defined(__ppc__)
      LSWAP(buf_union.val_uint);
#endif

      version = buf_union.val_uint;
      
      m_CurrentChunk->bytesRead += num_read;
      
      // If the file version is over 3, give a warning that there could be a problem
      if (version > 0x03)
        TRACE("Warning: This 3DS file is over version 3"
              " so it may load incorrectly\n");
      break;
      
    case OBJECTINFO:                        // This holds the version of the mesh
      
      // This chunk holds the version of the mesh.  It is also the
      // head of the MATERIAL and OBJECT chunks.  From here on we
      // start reading in the material and object info.
      
      // Read the next chunk
      ReadChunk(m_TempChunk);
      
      num_to_read = m_TempChunk->length - m_TempChunk->bytesRead;
      assert1(num_to_read <= BUF_SIZE);
      TRACE_FILE_IF(2)
        TRACE("Reading mesh VERSION: %d bytes\n", num_to_read);
      // Get the version of the mesh
      num_read = fread(&buf_union, 1, num_to_read, m_FilePointer);
      assert1(num_read == num_to_read);
#if defined(__ppc__)
      LSWAP(buf_union.val_uint);
#endif

      version = buf_union.val_uint;
      m_TempChunk->bytesRead += num_read;
      
      // Increase the bytesRead by the bytes read from the last chunk
      m_CurrentChunk->bytesRead += m_TempChunk->bytesRead;
      
      // Go to the next chunk, which is the object has a texture, it
      // should be MATERIAL, then OBJECT.
      ProcessNextChunk(pModel, m_CurrentChunk);
      break;
      
    case MATERIAL:                     // This holds the material information
      
      // This chunk is the header for the material info chunks
      
      // Increase the number of materials
      pModel->numOfMaterials++;
      
      // Add a empty texture structure to our texture list.  If you
      // are unfamiliar with STL's "vector" class, all push_back()
      // does is add a new node onto the list.  I used the vector
      // class so I didn't need to write my own link list functions.
      pModel->pMaterials.push_back(newTexture);
      
      // Proceed to the material loading function
      ProcessNextMaterialChunk(pModel, m_CurrentChunk);
      break;
      
    case OBJECT:                    // This holds the name of the object being read
      
      // This chunk is the header for the object info chunks.  It also
      // holds the name of the object.
      
      // Increase the object count
      pModel->numOfObjects++;
      
      // Add a new tObject node to our list of objects (like a link list)
      pModel->pObject.push_back(newObject);
      
      // Initialize the object and all it's data members
      memset(&(pModel->pObject[pModel->numOfObjects - 1]), 0, sizeof(t3DObject));
      
      // Get the name of the object and store it, then add the read
      // bytes to our byte counter.
      m_CurrentChunk->bytesRead += GetString(pModel->pObject[pModel->numOfObjects - 1].strName);
      
      // Now proceed to read in the rest of the object information
      ProcessNextObjectChunk(pModel, &(pModel->pObject[pModel->numOfObjects - 1]), m_CurrentChunk);
      break;
      
    case EDITKEYFRAME:
      
      // Because I wanted to make this a SIMPLE tutorial as possible,
      // I did not include the key frame information.  This chunk is
      // the header for all the animation info.  In a later tutorial
      // this will be the subject and explained thoroughly.
      
      //ProcessNextKeyFrameChunk(pModel, m_CurrentChunk);
      
      // Read past this chunk and add the bytes read to the byte counter
      num_read = fread(&buffer[0], 1, m_CurrentChunk->length - m_CurrentChunk->bytesRead, m_FilePointer);
      m_CurrentChunk->bytesRead += num_read;
      assert1(num_read <= BUF_SIZE);
      break;
      
    default: 
      
      // If we didn't care about a chunk, then we get here.  We still
      // need to read past the unknown or ignored chunk and add the
      // bytes read to the byte counter.
      num_read = fread(&buffer[0], 1, m_CurrentChunk->length - m_CurrentChunk->bytesRead, m_FilePointer);
      m_CurrentChunk->bytesRead += num_read;
      assert1(num_read <= BUF_SIZE);
      break;
    }
    
    // Add the bytes read from the last chunk to the previous chunk passed in.
    pPreviousChunk->bytesRead += m_CurrentChunk->bytesRead;
  }
  
  // Free the current chunk and set it back to the previous chunk
  // (since it started that way)
  delete m_CurrentChunk;
  m_CurrentChunk = pPreviousChunk;
}


//////////////////////// PROCESS NEXT OBJECT CHUNK \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function handles all the information about the objects in the file
/////
//////////////////////// PROCESS NEXT OBJECT CHUNK \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void CLoad3DS::ProcessNextObjectChunk(t3DModel *pModel, t3DObject *pObject, tChunk *pPreviousChunk)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("pModel = %p, pObject = %p, chunk = %p\n",
                           pModel, pObject, pPreviousChunk);}
  unsigned num_read;
  vector<int> buffer(BUF_SIZE); // This is used to read past unwanted data

  // Allocate a new chunk to work with
  m_CurrentChunk = new tChunk;
  
  // Continue to read these chunks until we read the end of this sub chunk
  while (pPreviousChunk->bytesRead < pPreviousChunk->length)
  {
    // Read the next chunk
    ReadChunk(m_CurrentChunk);
    
    // Check which chunk we just read
    switch (m_CurrentChunk->ID)
    {
    case OBJECT_MESH:                    // This lets us know that we are reading a new object
      
      // We found a new object, so let's read in it's info using recursion
      ProcessNextObjectChunk(pModel, pObject, m_CurrentChunk);
      break;
      
    case OBJECT_VERTICES:                // This is the objects vertices
      ReadVertices(pObject, m_CurrentChunk);
      break;
      
    case OBJECT_FACES:                    // This is the objects face information
      ReadVertexIndices(pObject, m_CurrentChunk);
      break;
      
    case OBJECT_MATERIAL:                // This holds the material name that the object has
      
      // This chunk holds the name of the material that the object has
      // assigned to it.  This could either be just a color or a
      // texture map.  This chunk also holds the faces that the
      // texture is assigned to (In the case that there is multiple
      // textures assigned to one object, or it just has a texture on
      // a part of the object.  Since most of my game objects just
      // have the texture around the whole object, and they aren't
      // multitextured, I just want the material name.
      
      // We now will read the name of the material assigned to this object
      ReadObjectMaterial(pModel, pObject, m_CurrentChunk);            
      break;
      
    case OBJECT_UV:        // This holds the UV texture coordinates for the object
      
      // This chunk holds all of the UV coordinates for our object.
      // Let's read them in.
      ReadUVCoordinates(pObject, m_CurrentChunk);
      break;
      
    default:  
      
      // Read past the ignored or unknown chunks
      num_read = fread(&buffer[0], 1, m_CurrentChunk->length - m_CurrentChunk->bytesRead, m_FilePointer);
      m_CurrentChunk->bytesRead += num_read;
      assert1(num_read <= BUF_SIZE);
      break;
    }
    
    // Add the bytes read from the last chunk to the previous chunk passed in.
    pPreviousChunk->bytesRead += m_CurrentChunk->bytesRead;
  }
  
  // Free the current chunk and set it back to the previous chunk
  // (since it started that way)
  delete m_CurrentChunk;
  m_CurrentChunk = pPreviousChunk;
}


////////////////// PROCESS NEXT MATERIAL CHUNK \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function handles all the information about the material (Texture)
/////
////////////////// PROCESS NEXT MATERIAL CHUNK \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void CLoad3DS::ProcessNextMaterialChunk(t3DModel *pModel, tChunk *pPreviousChunk)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("pModel = %p, chunk = %p\n",
                           pModel, pPreviousChunk);}
  unsigned num_read, num_to_read;
  vector<int> buffer(BUF_SIZE); // This is used to read past unwanted data

  // Allocate a new chunk to work with
  m_CurrentChunk = new tChunk;
  
  // Continue to read these chunks until we read the end of this sub chunk
  while (pPreviousChunk->bytesRead < pPreviousChunk->length)
  {
    // Read the next chunk
    ReadChunk(m_CurrentChunk);
    
    // Check which chunk we just read in
    switch (m_CurrentChunk->ID)
    {
    case MATNAME:                // This chunk holds the name of the material
      
      // Here we read in the material name
      num_to_read = m_CurrentChunk->length - m_CurrentChunk->bytesRead;
      assert1(num_to_read <= 255);
      m_CurrentChunk->bytesRead += fread(pModel->pMaterials[pModel->numOfMaterials - 1].strName, 1, num_to_read, m_FilePointer);
      TRACE_FILE_IF(1)
        TRACE("Loading material %s\n", 
              pModel->pMaterials[pModel->numOfMaterials - 1].strName);
      break;
      
    case MATDIFFUSE:             // This holds the R G B color of our object
      ReadColorChunk(&(pModel->pMaterials[pModel->numOfMaterials - 1]),
                     m_CurrentChunk);
      break;
      
    case MATMAP:                // This is the header for the texture info
      
      // Proceed to read in the material information
      ProcessNextMaterialChunk(pModel, m_CurrentChunk);
      break;
      
    case MATMAPFILE:             // This stores the file name of the material
      
      // Here we read in the material's file name
      num_to_read = m_CurrentChunk->length - m_CurrentChunk->bytesRead;
      assert1(num_to_read <= 255);
      m_CurrentChunk->bytesRead += fread(pModel->pMaterials[pModel->numOfMaterials - 1].strFile, 1, num_to_read, m_FilePointer);
      break;
      
    default:  
      
      // Read past the ignored or unknown chunks
      num_read = fread(&buffer[0], 1, m_CurrentChunk->length - m_CurrentChunk->bytesRead, m_FilePointer);
      m_CurrentChunk->bytesRead += num_read;
      assert1(num_read <= BUF_SIZE);
      break;
    }
    
    // Add the bytes read from the last chunk to the previous chunk passed in.
    pPreviousChunk->bytesRead += m_CurrentChunk->bytesRead;
  }
  
  // Free the current chunk and set it back to the previous chunk
  // (since it started that way)
  delete m_CurrentChunk;
  m_CurrentChunk = pPreviousChunk;
}



///////////////////////////////// READ CHUNK \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function reads in a chunk ID and it's length in bytes
/////
///////////////////////////////// READ CHUNK \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void CLoad3DS::ReadChunk(tChunk *pChunk)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("chunk = %p\n",
                           pChunk);}
  // This reads the chunk ID which is 2 bytes.
  // The chunk ID is like OBJECT or MATERIAL.  It tells what data is
  // able to be read in within the chunks section.  
  pChunk->bytesRead = fread(&pChunk->ID, 1, 2, m_FilePointer);
#if defined(__ppc__)
  SSWAP(pChunk->ID);
#endif
  
  // Then, we read the length of the chunk which is 4 bytes.
  // This is how we know how much to read in, or read past.
  pChunk->bytesRead += fread(&pChunk->length, 1, 4, m_FilePointer);
#if defined(__ppc__)
  LSWAP(pChunk->length);
#endif

  TRACE_FILE_IF(2)
    show_chunk(pChunk);
}

///////////////////////////////// GET STRING \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function reads in a string of characters
/////
///////////////////////////////// GET STRING \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

int CLoad3DS::GetString(char *pBuffer)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("pBuffer = %p\n",
                           pBuffer);}
  int index = 0;
  
  // Read 1 byte of data which is the first letter of the string
  fread(pBuffer, 1, 1, m_FilePointer);
  
  // Loop until we get NULL
  while (*(pBuffer + index++) != 0) {
    
    // Read in a character at a time until we hit NULL.
    fread(pBuffer + index, 1, 1, m_FilePointer);
  }
  assert1(index <= 255);
  
  // Return the string length, which is how many bytes we read in
  // (including the NULL)
  return strlen(pBuffer) + 1;
}


///////////////////////////////// READ COLOR \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function reads in the RGB color data
/////
///////////////////////////////// READ COLOR \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void CLoad3DS::ReadColorChunk(tMaterialInfo *pMaterial, 
                              tChunk *pChunk)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("pMaterial = %p, chunk = %p\n",
                           pMaterial, pChunk);}
  unsigned int num_to_read;
   
  // Read the color chunk info
  ReadChunk(m_TempChunk);
  
  // Read in the R G B color (3 bytes - 0 through 255)
  // sometimes the number to read is more than 3...
  num_to_read = m_TempChunk->length - m_TempChunk->bytesRead;
  if (num_to_read == 3)
  {
    m_TempChunk->bytesRead += fread(pMaterial->color, 1, num_to_read, m_FilePointer);
  }
  else if (num_to_read == 12)
  {
    // assume it's floats?
    m_TempChunk->bytesRead += fread(&buf_union, 1, num_to_read, m_FilePointer);
    // copy
    for (int i = 0 ; i < 3 ; ++i)
    {
#if defined(__ppc__)
      FSWAP(buf_union.fbuf[i]);
#endif
      pMaterial->color[i] = (unsigned char) (buf_union.fbuf[i] * 255);
    }
  }
  else
  {
    TRACE("num to read = %d!\n", num_to_read);
    pMaterial->color[0] = 255;
    pMaterial->color[1] = 0;
    pMaterial->color[2] = 0;
  }
  
  // alpha may be over-ridden later if an object name starting with
  // trans uses this material
  pMaterial->color[3] = 255;

  // Add the bytes read to our chunk
  pChunk->bytesRead += m_TempChunk->bytesRead;
}


/////////////////////// READ VERTEX INDECES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function reads in the indices for the vertex array
/////
/////////////////////// READ VERTEX INDECES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void CLoad3DS::ReadVertexIndices(t3DObject *pObject, tChunk *pPreviousChunk)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("pObject = %p, chunk = %p\n",
                           pObject, pPreviousChunk);}
  unsigned short index = 0;        // This is used to read in the current face index
  
  // In order to read in the vertex indices for the object, we need to
  // first read in the number of them, then read them in.  Remember,
  // we only want 3 of the 4 values read in for each face.  The fourth
  // is a visibility flag for 3D Studio Max that doesn't mean anything
  // to us.
  
  // Read in the number of faces that are in this object (short)
  pPreviousChunk->bytesRead += fread(&pObject->numOfFaces, 1, 2, m_FilePointer);
#if defined(__ppc__)
  SSWAP(pObject->numOfFaces);
#endif
  
  // Alloc enough memory for the faces and initialize the structure
  pObject->pFaces = new tFace [pObject->numOfFaces];
  memset(pObject->pFaces, 0, sizeof(tFace) * pObject->numOfFaces);
  
  // Go through all of the faces in this object
  for(int i = 0; i < pObject->numOfFaces; i++)
  {
    // Next, we read in the A then B then C index for the face, but
    // ignore the 4th value.  The fourth value is a visibility flag
    // for 3D Studio Max, we don't care about this.
    for(int j = 0; j < 4; j++)
    {
      // Read the first vertice index for the current face 
      pPreviousChunk->bytesRead += fread(&index, 1, sizeof(index), m_FilePointer);
#if defined(__ppc__)
      SSWAP(index);
#endif
      
      if(j < 3)
      {
        // Store the index in our face structure.
        pObject->pFaces[i].vertIndex[j] = index;
      }
    }
  }
}


/////////////////////////// READ UV COORDINATES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function reads in the UV coordinates for the object
/////
/////////////////////////// READ UV COORDINATES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void CLoad3DS::ReadUVCoordinates(t3DObject *pObject, tChunk *pPreviousChunk)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("pObject = %p, chunk = %p\n",
                           pObject, pPreviousChunk);}
  // In order to read in the UV indices for the object, we need to
  // first read in the amount there are, then read them in.
  
  // Read in the number of UV coordinates there are (short)
  pPreviousChunk->bytesRead += fread(&pObject->numTexVertex, 1, 2, m_FilePointer);
#if defined(__ppc__)
  SSWAP(pObject->numTexVertex);
#endif
  
  // Allocate memory to hold the UV coordinates
  pObject->pTexVerts = new CVector2 [pObject->numTexVertex];
  
  // Read in the texture coodinates (an array 2 float)
  unsigned int num_to_read;
  num_to_read = pPreviousChunk->length - pPreviousChunk->bytesRead;
  assert1(num_to_read <= pObject->numTexVertex * sizeof(CVector2));
  pPreviousChunk->bytesRead += fread(pObject->pTexVerts, 1, num_to_read, m_FilePointer);
  //TRACE("pTexVerts num_to_read: %d size: %d\n", num_to_read, pObject->numTexVertex * sizeof(CVector2));
#if defined(__ppc__)
  for(int i=0; i<pObject->numTexVertex; i++) {
    FSWAP(pObject->pTexVerts[i].x);
    FSWAP(pObject->pTexVerts[i].y);
  }
#endif
}


///////////////////////////////// READ VERTICES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function reads in the vertices for the object
/////
///////////////////////////////// READ VERTICES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void CLoad3DS::ReadVertices(t3DObject *pObject, tChunk *pPreviousChunk)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("pObject = %p, chunk = %p\n",
                           pObject, pPreviousChunk);}
  // Like most chunks, before we read in the actual vertices, we need
  // to find out how many there are to read in.  Once we have that number
  // we then fread() them into our vertice array.
  
  // Read in the number of vertices (short)
  pPreviousChunk->bytesRead += fread(&(pObject->numOfVerts), 1, 2, m_FilePointer);
#if defined(__ppc__)
  SSWAP(pObject->numOfVerts);
#endif
  
  // Allocate the memory for the verts and initialize the structure
  pObject->pVerts = new CVector3 [pObject->numOfVerts];
  memset(pObject->pVerts, 0, sizeof(CVector3) * pObject->numOfVerts);
  
  // Read in the array of vertices (an array of 3 floats)
  unsigned int num_to_read;
  num_to_read = pPreviousChunk->length - pPreviousChunk->bytesRead;
  assert1(num_to_read <= pObject->numOfVerts * sizeof(CVector3));
  pPreviousChunk->bytesRead += fread(pObject->pVerts, 1, num_to_read, m_FilePointer);
  
  // Now we should have all of the vertices read in.  Because 3D
  // Studio Max Models with the Z-Axis pointing up (strange and ugly I
  // know!), we need to flip the y values with the z values in our
  // vertices.  That way it will be normal, with Y pointing up.  If
  // you prefer to work with Z pointing up, then just delete this next
  // loop.  Also, because we swap the Y and Z we need to negate the Z
  // to make it come out correctly.
  
  // Go through all of the vertices that we just read and swap the Y and Z values
  for(int i = 0; i < pObject->numOfVerts; i++)
  {
#if defined(__ppc__)
    FSWAP(pObject->pVerts[i].x);
    FSWAP(pObject->pVerts[i].y);
    FSWAP(pObject->pVerts[i].z);
#endif

    // Store off the Y value
    float fTempY = pObject->pVerts[i].y;
    
    // Set the Y value to the Z value
    pObject->pVerts[i].y = pObject->pVerts[i].z;
    
    // Set the Z value to the Y value, 
    // but negative Z because 3D Studio max does the opposite.
    pObject->pVerts[i].z = -fTempY;
  }
}


/////////////////////// READ OBJECT MATERIAL \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function reads in the material name assigned to the
/////    object and sets the materialID
/////
/////////////////////// READ OBJECT MATERIAL \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void CLoad3DS::ReadObjectMaterial(t3DModel *pModel, t3DObject *pObject, tChunk *pPreviousChunk)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("pModel = %p, pObject = %p, chunk = %p\n",
                           pModel, pObject, pPreviousChunk);}
  char strMaterial[255] = {0};     // This is used to hold the objects material name
  unsigned num_read;
  vector<int> buffer(BUF_SIZE); // This is used to read past unwanted data
  
  // *What is a material?* - A material is either the color or the
  // texture map of the object.  It can also hold other information
  // like the brightness, shine, etc... Stuff we don't really care
  // about.  We just want the color, or the texture map file name
  // really.
  
  // Here we read the material name that is assigned to the current
  // object.  strMaterial should now have a string of the material
  // name, like "Material #2" etc..
  pPreviousChunk->bytesRead += GetString(strMaterial);
  
  // Now that we have a material name, we need to go through all of
  // the materials and check the name against each material.  When we
  // find a material in our material list that matches this name we
  // just read in, then we assign the materialID of the object to that
  // material index.  You will notice that we passed in the model to
  // this function.  This is because we need the number of textures.
  // Yes though, we could have just passed in the model and not the
  // object too.
  TRACE_FILE_IF(1)
    TRACE("Looking for material = %s\n", strMaterial);
  
  // Go through all of the textures
  for(int i = 0; i < pModel->numOfMaterials; i++)
  {
    // If the material we just read in matches the current texture name
    if(strcmp(strMaterial, pModel->pMaterials[i].strName) == 0)
    {
      // Set the material ID to the current index 'i' and stop checking
      pObject->materialID = i;
      
      if (strncmp("trans", pObject->strName, 5) == 0)
      {
        pModel->pMaterials[i].color[3] = 100;
      }

      // Now that we found the material, check if it's a texture map.
      // If the strFile has a string length of 1 and over it's a
      // texture
      
      if(strlen(pModel->pMaterials[i].strFile) > 0) {
        
        // Set the object's flag to say it has a texture map to bind.
        pObject->bHasTexture = true;
        // and do the texture thing

        pModel->pMaterials[i].rgba_texture = 
          new Rgba_file_texture(pModel->pMaterials[i].strFile,
                                Rgba_file_texture::CLAMP,
                                Rgba_file_texture::RGBA);
          
        /*
          pModel->pMaterials[i].rgba_texture = 
          new Rgba_file_texture("face.png",
          Rgba_file_texture::REPEAT,
          Rgba_file_texture::RGBA);
        */

        if (0 == pModel->pMaterials[i].rgba_texture->get_w())
        {
          delete pModel->pMaterials[i].rgba_texture;
          pModel->pMaterials[i].rgba_texture = 0;
          pObject->bHasTexture = false;
        }
      }    
      break;
    }
    else
    {
      // Set the ID to -1 to show there is no material for this object
      pObject->materialID = -1;
    }
  }

  if (pObject->materialID == -1)
  {
    TRACE("No material for object %p - wanted %s\n", pObject, strMaterial);
  }
  
  // Read past the rest of the chunk since we don't care about shared
  // vertices You will notice we subtract the bytes already read in
  // this chunk from the total length.
  num_read = fread(&buffer[0], 1, pPreviousChunk->length - pPreviousChunk->bytesRead, m_FilePointer);
  pPreviousChunk->bytesRead += num_read;
  assert1(num_read <= BUF_SIZE);
}            

// *Note* 
//
// Below are some math functions for calculating vertex normals.  We
// want vertex normals because it makes the lighting look really
// smooth and life like.  You probably already have these functions in
// the rest of your engine, so you can delete these and call your own.
// I wanted to add them so I could show how to calculate vertex
// normals.

//////////////////////////////    Math Functions  ////////////////////////////////*

// This computes the magnitude of a normal.   (magnitude = sqrt(x^2 + y^2 + z^2)
#define Mag(Normal) (sqrt(Normal.x*Normal.x + Normal.y*Normal.y + Normal.z*Normal.z))

// This calculates a vector between 2 points and returns the result
CVector3 Vector(CVector3 vPoint1, CVector3 vPoint2)
{
  CVector3 vVector;                  // The variable to hold the resultant vector
  
  vVector.x = vPoint1.x - vPoint2.x;            // Subtract point1 and point2 x's
  vVector.y = vPoint1.y - vPoint2.y;            // Subtract point1 and point2 y's
  vVector.z = vPoint1.z - vPoint2.z;            // Subtract point1 and point2 z's
  
  return vVector;                                // Return the resultant vector
}

// This adds 2 vectors together and returns the result
CVector3 AddVector(CVector3 vVector1, CVector3 vVector2)
{
  CVector3 vResult;                // The variable to hold the resultant vector
  
  vResult.x = vVector2.x + vVector1.x;        // Add Vector1 and Vector2 x's
  vResult.y = vVector2.y + vVector1.y;        // Add Vector1 and Vector2 y's
  vResult.z = vVector2.z + vVector1.z;        // Add Vector1 and Vector2 z's
  
  return vResult;                                // Return the resultant vector
}

// This divides a vector by a single number (scalar) and returns the result
CVector3 DivideVectorByScaler(CVector3 vVector1, float Scaler)
{
  CVector3 vResult;                 // The variable to hold the resultant vector
  
  vResult.x = vVector1.x / Scaler;         // Divide Vector1's x value by the scaler
  vResult.y = vVector1.y / Scaler;         // Divide Vector1's y value by the scaler
  vResult.z = vVector1.z / Scaler;         // Divide Vector1's z value by the scaler
  
  return vResult;                          // Return the resultant vector
}

// This returns the cross product between 2 vectors
CVector3 Cross(CVector3 vVector1, CVector3 vVector2)
{
  CVector3 vCross;                       // The vector to hold the cross product
  // Get the X value
  vCross.x = ((vVector1.y * vVector2.z) - (vVector1.z * vVector2.y));
  // Get the Y value
  vCross.y = ((vVector1.z * vVector2.x) - (vVector1.x * vVector2.z));
  // Get the Z value
  vCross.z = ((vVector1.x * vVector2.y) - (vVector1.y * vVector2.x));
  
  return vCross;                                // Return the cross product
}

// This returns the normal of a vector
CVector3 Normalize(CVector3 vNormal)
{
  double Magnitude;                     // This holds the magitude            
  
  Magnitude = Mag(vNormal);             // Get the magnitude
  
  vNormal.x /= (float)Magnitude;        // Divide the vector's X by the magnitude
  vNormal.y /= (float)Magnitude;        // Divide the vector's Y by the magnitude
  vNormal.z /= (float)Magnitude;        // Divide the vector's Z by the magnitude
  
  return vNormal;                       // Return the normal
}

////////////////////////// COMPUTER NORMALS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////    This function computes the normals and vertex normals of the objects
/////
////////////////////////// COMPUTER NORMALS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void CLoad3DS::ComputeNormals(t3DModel *pModel)
{
  TRACE_FILE_IF(2) {
    TRACE_METHOD() ; TRACE("pModel = %p\n", pModel);}
  CVector3 vVector1, vVector2, vNormal, vPoly[3];
  
  // If there are no objects, we can skip this part
  if(pModel->numOfObjects <= 0)
    return;
  
  // What are vertex normals?  And how are they different from other
  // normals?  Well, if you find the normal to a triangle, you are
  // finding a "Face Normal".  If you give OpenGL a face normal for
  // lighting, it will make your object look really flat and not very
  // round.  If we find the normal for each vertex, it makes the
  // smooth lighting look.  This also covers up blocky looking objects
  // and they appear to have more polygons than they do.  Basically,
  // what you do is first calculate the face normals, then you take
  // the average of all the normals around each vertex.  It's just
  // averaging.  That way you get a better approximation for that
  // vertex.
  
  // Go through each of the objects to calculate their normals
  for(int index = 0; index < pModel->numOfObjects; index++)
  {
    // Get the current object
    t3DObject *pObject = &(pModel->pObject[index]);
    
    // Here we allocate all the memory we need to calculate the normals
    CVector3 *pNormals        = new CVector3 [pObject->numOfFaces];
    CVector3 *pTempNormals    = new CVector3 [pObject->numOfFaces];
    pObject->pNormals        = new CVector3 [pObject->numOfVerts];
    
    // Go though all of the faces of this object
    int i;
    for(i=0; i < pObject->numOfFaces; i++)
    {                                                
      // To cut down LARGE code, we extract the 3 points of this face
      vPoly[0] = pObject->pVerts[pObject->pFaces[i].vertIndex[0]];
      vPoly[1] = pObject->pVerts[pObject->pFaces[i].vertIndex[1]];
      vPoly[2] = pObject->pVerts[pObject->pFaces[i].vertIndex[2]];
      
      // Now let's calculate the face normals (Get 2 vectors and find
      // the cross product of those 2)
      
      // Get the vector of the polygon (we just need 2 sides for the
      // normal)
      vVector1 = Vector(vPoly[0], vPoly[2]); 
      vVector2 = Vector(vPoly[2], vPoly[1]); // Get a second vector of the polygon
      
      // Return the cross product of the 2 vectors (normalize vector,
      // but not a unit vector)
      vNormal  = Cross(vVector1, vVector2);  
      pTempNormals[i] = vNormal;// Save the un-normalized normal for the vertex normals

      // Normalize the cross product to give us the polygons normal
      vNormal  = Normalize(vNormal); 
      
      pNormals[i] = vNormal; // Assign the normal to the list of normals
    }
    
    //////////////// Now Get The Vertex Normals /////////////////
    
    CVector3 vSum = {0.0, 0.0, 0.0};
    CVector3 vZero = vSum;
    int shared=0;
    
    for (i = 0; i < pObject->numOfVerts; i++)    // Go through all of the vertices
    {
      for (int j = 0; j < pObject->numOfFaces; j++) // Go through all of the triangles
      {                            // Check if the vertex is shared by another face
        if (pObject->pFaces[j].vertIndex[0] == i || 
            pObject->pFaces[j].vertIndex[1] == i || 
            pObject->pFaces[j].vertIndex[2] == i)
        {
          vSum = AddVector(vSum, pTempNormals[j]);// Add the un-normalized normal of the shared face
          shared++;                   // Increase the number of shared triangles
        }
      }      
      
      // Get the normal by dividing the sum by the shared.  We negate
      // the shared so it has the normals pointing out.
      pObject->pNormals[i] = DivideVectorByScaler(vSum, float(-shared));
      
      // Normalize the normal for the final vertex normal
      pObject->pNormals[i] = Normalize(pObject->pNormals[i]);    
      
      vSum = vZero;                                    // Reset the sum
      shared = 0;                                      // Reset the shared
    }
    
    // Free our memory and start over on the next object
    delete [] pTempNormals;
    delete [] pNormals;
  }
}


/////////////////////////////////////////////////////////////////////////////////
//
// * QUICK NOTES * 
//
// This was a HUGE amount of knowledge and probably the largest
// tutorial yet!  In the next tutorial we will show you how to load a
// text file format called .obj.  This is the most common 3D file
// format that almost ANY 3D software will import.
//
// Once again I should point out that the coordinate system of OpenGL
// and 3DS Max are different.  Since 3D Studio Max Models with the
// Z-Axis pointing up (strange and ugly I know! :), we need to flip
// the y values with the z values in our vertices.  That way it will
// be normal, with Y pointing up.  Also, because we swap the Y and Z
// we need to negate the Z to make it come out correctly.  This is
// also explained and done in ReadVertices().
//
// CHUNKS: What is a chunk anyway?
// 
// "The chunk ID is a unique code which identifies the type of data in
// this chunk and also may indicate the existence of subordinate
// chunks. The chunk length indicates the length of following data to
// be associated with this chunk. Note, this may contain more data
// than just this chunk. If the length of data is greater than that
// needed to fill in the information for the chunk, additional
// subordinate chunks are attached to this chunk immediately following
// any data needed for this chunk, and should be parsed out. These
// subordinate chunks may themselves contain subordinate chunks.
// Unfortunately, there is no indication of the length of data, which
// is owned by the current chunk, only the total length of data
// attached to the chunk, which means that the only way to parse out
// subordinate chunks is to know the exact format of the owning
// chunk. On the other hand, if a chunk is unknown, the parsing
// program can skip the entire chunk and subordinate chunks in one
// jump. " - Jeff Lewis (werewolf@worldgate.com)
//
// In a short amount of words, a chunk is defined this way:
// 2 bytes - Stores the chunk ID (OBJECT, MATERIAL, PRIMARY, etc...)
// 4 bytes - Stores the length of that chunk.  That way you know when that
//           chunk is done and there is a new chunk.
//
// So, to start reading the 3DS file, you read the first 2 bytes of
// it, then the length (using fread()).  It should be the PRIMARY
// chunk, otherwise it isn't a .3DS file.
//
// Below is a list of the order that you will find the chunks and all
// the know chunks.  If you go to www.wosit.org you can find a few
// documents on the 3DS file format.  You can also take a look at the
// 3DS Format.rtf that is included with this tutorial.
//
//
//
//      MAIN3DS  (0x4D4D)
//     |
//     +--EDIT3DS  (0x3D3D)
//     |  |
//     |  +--EDIT_MATERIAL (0xAFFF)
//     |  |  |
//     |  |  +--MAT_NAME01 (0xA000) (See mli Doc) 
//     |  |
//     |  +--EDIT_CONFIG1  (0x0100)
//     |  +--EDIT_CONFIG2  (0x3E3D) 
//     |  +--EDIT_VIEW_P1  (0x7012)
//     |  |  |
//     |  |  +--TOP            (0x0001)
//     |  |  +--BOTTOM         (0x0002)
//     |  |  +--LEFT           (0x0003)
//     |  |  +--RIGHT          (0x0004)
//     |  |  +--FRONT          (0x0005) 
//     |  |  +--BACK           (0x0006)
//     |  |  +--USER           (0x0007)
//     |  |  +--CAMERA         (0xFFFF)
//     |  |  +--LIGHT          (0x0009)
//     |  |  +--DISABLED       (0x0010)  
//     |  |  +--BOGUS          (0x0011)
//     |  |
//     |  +--EDIT_VIEW_P2  (0x7011)
//     |  |  |
//     |  |  +--TOP            (0x0001)
//     |  |  +--BOTTOM         (0x0002)
//     |  |  +--LEFT           (0x0003)
//     |  |  +--RIGHT          (0x0004)
//     |  |  +--FRONT          (0x0005) 
//     |  |  +--BACK           (0x0006)
//     |  |  +--USER           (0x0007)
//     |  |  +--CAMERA         (0xFFFF)
//     |  |  +--LIGHT          (0x0009)
//     |  |  +--DISABLED       (0x0010)  
//     |  |  +--BOGUS          (0x0011)
//     |  |
//     |  +--EDIT_VIEW_P3  (0x7020)
//     |  +--EDIT_VIEW1    (0x7001) 
//     |  +--EDIT_BACKGR   (0x1200) 
//     |  +--EDIT_AMBIENT  (0x2100)
//     |  +--EDIT_OBJECT   (0x4000)
//     |  |  |
//     |  |  +--OBJ_TRIMESH   (0x4100)      
//     |  |  |  |
//     |  |  |  +--TRI_VERTEXL          (0x4110) 
//     |  |  |  +--TRI_VERTEXOPTIONS    (0x4111)
//     |  |  |  +--TRI_MAPPINGCOORS     (0x4140) 
//     |  |  |  +--TRI_MAPPINGSTANDARD  (0x4170)
//     |  |  |  +--TRI_FACEL1           (0x4120)
//     |  |  |  |  |
//     |  |  |  |  +--TRI_SMOOTH            (0x4150)   
//     |  |  |  |  +--TRI_MATERIAL          (0x4130)
//     |  |  |  |
//     |  |  |  +--TRI_LOCAL            (0x4160)
//     |  |  |  +--TRI_VISIBLE          (0x4165)
//     |  |  |
//     |  |  +--OBJ_LIGHT    (0x4600)
//     |  |  |  |
//     |  |  |  +--LIT_OFF              (0x4620)
//     |  |  |  +--LIT_SPOT             (0x4610) 
//     |  |  |  +--LIT_UNKNWN01         (0x465A) 
//     |  |  | 
//     |  |  +--OBJ_CAMERA   (0x4700)
//     |  |  |  |
//     |  |  |  +--CAM_UNKNWN01         (0x4710)
//     |  |  |  +--CAM_UNKNWN02         (0x4720)  
//     |  |  |
//     |  |  +--OBJ_UNKNWN01 (0x4710)
//     |  |  +--OBJ_UNKNWN02 (0x4720)
//     |  |
//     |  +--EDIT_UNKNW01  (0x1100)
//     |  +--EDIT_UNKNW02  (0x1201) 
//     |  +--EDIT_UNKNW03  (0x1300)
//     |  +--EDIT_UNKNW04  (0x1400)
//     |  +--EDIT_UNKNW05  (0x1420)
//     |  +--EDIT_UNKNW06  (0x1450)
//     |  +--EDIT_UNKNW07  (0x1500)
//     |  +--EDIT_UNKNW08  (0x2200)
//     |  +--EDIT_UNKNW09  (0x2201)
//     |  +--EDIT_UNKNW10  (0x2210)
//     |  +--EDIT_UNKNW11  (0x2300)
//     |  +--EDIT_UNKNW12  (0x2302)
//     |  +--EDIT_UNKNW13  (0x2000)
//     |  +--EDIT_UNKNW14  (0xAFFF)
//     |
//     +--KEYF3DS (0xB000)
//        |
//        +--KEYF_UNKNWN01 (0xB00A)
//        +--............. (0x7001) ( viewport, same as editor )
//        +--KEYF_FRAMES   (0xB008)
//        +--KEYF_UNKNWN02 (0xB009)
//        +--KEYF_OBJDES   (0xB002)
//           |
//           +--KEYF_OBJHIERARCH  (0xB010)
//           +--KEYF_OBJDUMMYNAME (0xB011)
//           +--KEYF_OBJUNKNWN01  (0xB013)
//           +--KEYF_OBJUNKNWN02  (0xB014)
//           +--KEYF_OBJUNKNWN03  (0xB015)  
//           +--KEYF_OBJPIVOT     (0xB020)  
//           +--KEYF_OBJUNKNWN04  (0xB021)  
//           +--KEYF_OBJUNKNWN05  (0xB022)  
//
// Once you know how to read chunks, all you have to know is the ID
// you are looking for and what data is stored after that ID.  You
// need to get the file format for that.  I can give it to you if you
// want, or you can go to www.wosit.org for several versions.  Because
// this is a proprietary format, it isn't a official document.
//
// I know there was a LOT of information blown over, but it is too
// much knowledge for one tutorial.  In the animation tutorial that I
// eventually will get to, some of the things explained here will be
// explained in more detail.  I do not claim that this is the best
// .3DS tutorial, or even a GOOD one :) But it is a good start, and
// there isn't much code out there that is simple when it comes to
// reading .3DS files.  So far, this is the best I have seen.  That is
// why I made it :)
// 
// I would like to thank www.wosit.org and Terry Caton
// (tcaton@umr.edu) for his help on this.
//
// Let me know if this helps you out!
// 
// Ben Humphrey (DigiBen)
// Game Programmer
// DigiBen@GameTutorials.com
// Co-Web Host of www.GameTutorials.com
