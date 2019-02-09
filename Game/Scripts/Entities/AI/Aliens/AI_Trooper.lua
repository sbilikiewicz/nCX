Script.ReloadScript( "SCRIPTS/Entities/AI/Aliens/AI_Trooper_x.lua");

CreateAlien(AI_Trooper_x);
AI_Trooper = CreateAI(AI_Trooper_x)
AI_Trooper:Expose();
