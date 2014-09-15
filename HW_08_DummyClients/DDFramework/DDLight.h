#pragma once
#include "DDObject.h"
class DDLight :
	public DDObject
{
public:
	DDLight();
	virtual ~DDLight();

	CREATE_OBJECT( DDLight );
	//static DDLight* Create();

private:
	virtual void RenderItSelf();
};

