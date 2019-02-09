Script.ReloadScript( "SCRIPTS/Entities/AI/AI_Grunt_x.lua");

CreateActor(AI_Grunt_x);
AI_Grunt = CreateAI(AI_Grunt_x);
AI_Grunt:Expose();
--MakeSpawnable(CoopGrunt)

