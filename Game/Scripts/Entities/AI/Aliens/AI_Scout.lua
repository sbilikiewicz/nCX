Script.ReloadScript( "SCRIPTS/Entities/AI/Aliens/AI_Scout_x.lua");

CreateAlien(AI_Scout_x);
AI_Scout = CreateAI(AI_Scout_x)
AI_Scout:Expose();
