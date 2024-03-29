----------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios.
----------------------------------------------------------------------
--  BasicAIEvents.lua
--  Moved out all the events from BasicAI
----------------------------------------------------------------------
--  History:
--  06/2005  :  Created by Kirill Bulatsev
--  02/2019  :  Edited and optimized by sbilikiewicz
--              https://github.com/sbilikiewicz
----------------------------------------------------------------------
BasicAIEvent = {}

MakeUsable(BasicAIEvent);

function BasicAIEvent:Event_Dead(params)
	BroadcastEvent(self, "Dead");
end

function BasicAIEvent:Event_WakeUp(params)
	if (self.actor:GetPhysicalizationProfile() == "sleep") then
		self.actor:StandUp();
	end
end

function BasicAIEvent:Event_Sleep(params)
	if(not self.isFallen) then
		BroadcastEvent(self, "Sleep");
	end	
	self.isFallen = 1;
end

function BasicAIEvent:Event_Enabled( params )
	BroadcastEvent(self, "Enabled");
end

function BasicAIEvent:Event_OnAlert( params )
	BroadcastEvent( self, "OnAlert" );
end

function BasicAIEvent:Event_Disable(params)
	self:Hide(1);
	-- sbilikiewicz : this event caused problems in mp
	-- self:TriggerEvent(AIEVENT_DISABLE);
end

function BasicAIEvent:Event_Enable(params)
	if (not self:IsDead() ) then 
		-- hide does enable/disable physics as well
		self:Hide(0)
		self:Event_Enabled(self);
		if(self.voiceTable and self.PlayIdleSound) then 
			if (self.cloaked == 1 and self.voiceTable.idleCloak) then
				self:PlayIdleSound(self.voiceTable.idleCloak);
			elseif(self.voiceTable.idle) then 
				self:PlayIdleSound(self.voiceTable.idle);
			end
		end
	end
end

function BasicAIEvent:Event_Kill(params) -- kill the actor
	if (not self:IsDead()) then 
		g_gameRules:CreateHit(self.id,self.id,self.id,100000,nil,nil,nil,"event");
	end
end

function BasicAIEvent:Event_Follow(sender)
	BroadcastEvent(self, "Follow");
	local newGroupId;
	if(sender.Who and sender.Who.id and sender.Who.id~=NULL_ENTITY) then -- it's a trigger
		newGroupId = AI.GetGroupOf(sender.Who.id);
	else
		newGroupId = AI.GetGroupOf(sender.id);
	end
	AI.ChangeParameter(self.id,AIPARAM_GROUPID,newGroupId);
	AI.Signal(SIGNALFILTER_SENDER,0,"FOLLOW_LEADER",self.id);
end

function BasicAIEvent:Event_Test(sender)
	g_SignalData.fValue = 2;
	AI.Signal(SIGNALFILTER_LEADER,0,"OnScaleFormation",self.id,g_SignalData);
end

function BasicAIEvent:Event_TestStealth(sender)
	AI.SetPFProperties(self.id, AIPATH_HUMAN_COVER);
end

BasicAIEvent.FlowEvents = {
	Inputs = {
		Used          = { BasicAIEvent.Event_Used, "bool" },
		EnableUsable  = { BasicAIEvent.Event_EnableUsable, "bool" },
		DisableUsable = { BasicAIEvent.Event_DisableUsable, "bool" },
		Disable       = { BasicAIEvent.Event_Disable, "bool" },
		Enable        = { BasicAIEvent.Event_Enable, "bool" },
		Kill          = { BasicAIEvent.Event_Kill, "bool" },
		WakeUp        = { BasicAIEvent.Event_WakeUp, "bool" },
	},
	Outputs = {
		Used      = "bool",
		Dead      = "bool",
		OnAlert   = "bool",
		Sleep     = "bool",
		Enabled   = "bool",
	},
}
