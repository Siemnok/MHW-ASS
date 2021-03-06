#include "stdafx.h"
#include "Decoration.h"
#include "Skill.h"
#include "Solution.h"

using namespace System;

void Decoration::Load( System::String^ filename )
{
	static_decoration_map.Clear();
	static_decoration_ability_map.Clear();
	static_decorations.Clear();
	static_decorations.Capacity = 100;
	IO::StreamReader fin( filename );
	while( !fin.EndOfStream )
	{
		String^ line = fin.ReadLine();
		if( line == L"" || line[ 0 ] == L'#' )
			continue;
		List_t< String^ > split;
		Utility::SplitString( %split, line, L',' );
		Decoration^ decoration = gcnew Decoration;
		decoration->is_event = false;
		decoration->num_owned = 0;
		//Name,Slot Level,Rarity,Acquire,Skill
		decoration->name = split[ 0 ];
		decoration->rarity = Convert::ToUInt32( split[ 2 ] );
		decoration->slot_level = Convert::ToUInt32( split[ 1 ] );
		decoration->hr = Convert::ToUInt32( split[ 3 ] );
		decoration->difficulty = 0;
		decoration->num_owned = 0;
		decoration->can_use = false;

		Ability^ ability = Ability::FindAbility( split[ 4 ] );
		Assert( ability, L"Cannot find decoration ability: " + split[4] );

		decoration->abilities.Add( gcnew AbilityPair( ability, 1 ) );
		Assert( !ability->decoration, L"Two decorations with the same ability" );
		ability->decoration = decoration;

		decoration->index = static_decorations.Count;
		static_decorations.Add( decoration );
		if( !static_decoration_ability_map.ContainsKey( decoration->abilities[ 0 ]->ability ) )
			static_decoration_ability_map.Add( decoration->abilities[ 0 ]->ability, gcnew List_t< Decoration^ > );
		static_decoration_ability_map[ decoration->abilities[ 0 ]->ability ]->Add( decoration );
		static_decoration_map.Add( decoration->name, decoration );
	}
	static_decorations.TrimExcess();
}

int Decoration::GetSkillAt( Ability^ ability )
{
	for( int i = 0; i < abilities.Count; ++i )
		if( abilities[ i ]->ability == ability )
			return abilities[ i ]->amount;
	return 0;
}

bool Decoration::IsBetterThan( Decoration^ other, List_t< Ability^ >^ rel_abilities )
{
	if( slot_level < other->slot_level || abilities[ 0 ]->ability != other->abilities[ 0 ]->ability )
		return true;

	return abilities[ 0 ]->amount > other->abilities[ 0 ]->amount;
}

Decoration^ Decoration::GetBestDecoration( Ability^ ability, const unsigned max_slot_level, const unsigned hr )
{
	if( !static_decoration_ability_map.ContainsKey( ability ) )
		return nullptr; //no decorations exist for this skill

	Decoration^ best = nullptr;
	List_t< Ability^ > rel;
	rel.Add( ability );
	for each( Decoration^ dec in static_decoration_ability_map[ ability ] )
	{
		if( dec->hr > hr || dec->slot_level > max_slot_level )
			continue;

		for each( AbilityPair^ ap in dec->abilities )
		{
			if( ap->amount > 0 && ap->ability == ability )
			{
				if( !best || dec->IsBetterThan( best, %rel ) )
					best = dec;
			}
		}
	}
	return best;
}

bool Decoration::MatchesQuery( Query^ query )
{
	//check requirements
	if( this->hr > query->hr )
	{
		can_use = false;
		return false;
	}

	can_use = true;

	//check for relevant skills
	for each( Skill^ skill in query->skills )
	{
		if( skill->ability == abilities[ 0 ]->ability )
			return true;
	}
	return false;
}

Decoration^ Decoration::FindDecoration( System::String^ name )
{
	if( static_decoration_map.ContainsKey( name ) )
		return static_decoration_map[ name ];
	return nullptr;
}

Decoration^ Decoration::FindDecorationFromString( System::String^ line )
{
	//1x Expert +5 Jewel
	const int plus = line->IndexOf( L'+' );
	const unsigned points = Convert::ToUInt32( line->Substring( plus + 1, 1 ) );

	Ability^ best_ability = nullptr;
	for each( Ability^ ability in Ability::static_abilities )
	{
		if( line->IndexOf( ability->name ) > -1 )
		{
			if( !best_ability || best_ability->name->Length < ability->name->Length )
				best_ability = ability;
		}
	}

	if( !best_ability )
		return nullptr;

	for each( Decoration^ dec in static_decorations )
	{
		if( dec->abilities[ 0 ]->ability == best_ability &&
			dec->abilities[ 0 ]->amount == points )
			return dec;
	}
	return nullptr;
}

Decoration^ Decoration::GetBestDecoration( Ability^ ability, const unsigned max_slots, List_t< List_t< Decoration^ >^ >% rel_deco_map )
{
	for( int i = max_slots + 1; i --> 0; )
	{
		for each( Decoration^ deco in rel_deco_map[ i ] )
		{
			if( deco->abilities[ 0 ]->ability == ability )
				return deco;
		}
	}
	return nullptr;
}

void Decoration::LoadLanguage( System::String^ filename )
{
	static_decoration_map.Clear();
	IO::StreamReader fin( filename );
	for( int i = 0; i < static_decorations.Count; )
	{
		String^ line = fin.ReadLine();
		if( line == L"" || line[ 0 ] == L'#' )
			continue;
		static_decorations[ i ]->name = line;
		static_decoration_map.Add( line, static_decorations[ i ] );
		i++;
	}
}

#define CUSTOM_LIST L"Data/mydecorations.txt"

void Decoration::LoadCustom()
{
	LoadCustom( CUSTOM_LIST );
}

void Decoration::LoadCustom( String^ filename )
{
	IO::StreamReader fin( filename );

	do
	{
		String^ line = fin.ReadLine();
		if( line == "" || line->StartsWith( L"#" ) )
			continue;

		int comma = line->IndexOf( L',' );
		if( comma < 0 )
		{
			Decoration^ d = FindDecoration( line->Trim() );
			if( d )
				d->num_owned++;
		}
		else
		{
			Decoration^ d = FindDecoration( line->Substring( 0, comma )->Trim() );
			if( d )
				d->num_owned += Convert::ToInt32( line->Substring( comma + 1 ) );
		}
	}
	while( !fin.EndOfStream );
}

void Decoration::SaveCustom()
{
	IO::StreamWriter fout( CUSTOM_LIST );

	fout.WriteLine( L"#Name,Number" );

	for each( Decoration^ d in static_decorations )
	{
		if( d->num_owned > 0 )
		{
			fout.WriteLine( d->name + L"," + d->num_owned );
		}
	}
}
