class cRoom;

class cObjectForcefield : public cObject {
public:

public:
	cObjectForcefield( cRoom *pRoom, byte pPosX, byte pPosY ) : cObject( pRoom, pPosX, pPosY ) {
		mObjectID = eObjectForcefield;
		mPartCount = 2;
	}

	size_t		objectLoad( byte **pBuffer ) {
		
		return 0;
	}

	size_t		objectSave( byte **pBuffer ) {	
		size_t strSize = cObject::objectSave( pBuffer );
		strSize += objectSave2( pBuffer );

		return (strSize);
	}

};
