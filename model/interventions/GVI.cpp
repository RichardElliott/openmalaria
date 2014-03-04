/* This file is part of OpenMalaria.
 * 
 * Copyright (C) 2005-2014 Swiss Tropical and Public Health Institute
 * Copyright (C) 2005-2014 Liverpool School Of Tropical Medicine
 * 
 * OpenMalaria is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "interventions/GVI.h"
#include "Host/Human.h"
#include "util/SpeciesIndexChecker.h"
#include "util/errors.h"
#include <cmath>

namespace OM { namespace interventions {

vector<GVIEffect*> GVIEffect::effectsByIndex;

GVIEffect::GVIEffect( EffectId id, const scnXml::GVIDescription& elt,
        const map<string,size_t>& species_name_map ) :
        Transmission::HumanVectorInterventionEffect(id)
{
    decay = DecayFunction::makeObject( elt.getDecay(), "interventions.human.vector.decay" );
    
    typedef scnXml::GVIDescription::AnophelesParamsSequence AP;
    const AP& ap = elt.getAnophelesParams();
    species.resize( species_name_map.size() );
    util::SpeciesIndexChecker checker( "GVI intervention", species_name_map );
    for( AP::const_iterator it = ap.begin(); it != ap.end(); ++it ) {
        species[checker.getIndex(it->getMosquito())].init (*it);
    }
    checker.checkNoneMissed();
    
    if( effectsByIndex.size() <= id.id ) effectsByIndex.resize( id.id+1, 0 );
    effectsByIndex[id.id] = this;
}

void GVIEffect::deploy( Host::Human& human, Deployment::Method method, VaccineLimits )const{
    human.perHostTransmission.deployEffect(*this);
    if( method == interventions::Deployment::TIMED ){
        Monitoring::Surveys.getSurvey(human.isInAnyCohort()).reportMassGVI( human.getMonitoringAgeGroup(), 1 );
    }else if( method == interventions::Deployment::CTS ){
        //TODO(monitoring): report
    }else throw SWITCH_DEFAULT_EXCEPTION;
}

Effect::Type GVIEffect::effectType()const{ return Effect::GVI; }
    
#ifdef WITHOUT_BOINC
void GVIEffect::print_details( std::ostream& out )const{
    out << id().id << "\tGVI";
}
#endif

PerHostInterventionData* GVIEffect::makeHumanPart() const{
    return new HumanGVI( *this );
}
PerHostInterventionData* GVIEffect::makeHumanPart( istream& stream, EffectId id ) const{
    return new HumanGVI( stream, id );
}

void GVIEffect::GVIAnopheles::init(const scnXml::GVIDescription::AnophelesParamsType& elt)
{
    assert( deterrency != deterrency ); // double init
    deterrency = elt.getDeterrency().present() ? elt.getDeterrency().get().getValue() : 0.0;
    preprandialKilling = elt.getPreprandialKillingEffect().present() ? elt.getPreprandialKillingEffect().get().getValue() : 0.0;
    postprandialKilling = elt.getPostprandialKillingEffect().present() ? elt.getPostprandialKillingEffect().get().getValue() : 0.0;
    // Simpler version of ITN usage/action:
    double propActive = elt.getPropActive();
    assert( propActive >= 0.0 && propActive <= 1.0 );
    proportionProtected = propActive;
    proportionUnprotected = 1.0 - proportionProtected;
}


// ———  per-human data  ———
HumanGVI::HumanGVI ( const GVIEffect& params ) :
    PerHostInterventionData( params.id() )
{
    // Varience factor of decay is sampled once per human: human is assumed
    // to account for most variance.
    decayHet = params.decay->hetSample();
}

void HumanGVI::redeploy(const Transmission::HumanVectorInterventionEffect&) {
    deployTime = TimeStep::simulation;
}

void HumanGVI::update(){
}

double HumanGVI::relativeAttractiveness(size_t speciesIndex) const{
    const GVIEffect& params = *GVIEffect::effectsByIndex[m_id.id];
    const GVIEffect::GVIAnopheles& anoph = params.species[speciesIndex];
    double effect = (1.0 - anoph.deterrency * getEffectSurvival(params));
    return anoph.byProtection( effect );
}

double HumanGVI::preprandialSurvivalFactor(size_t speciesIndex) const{
    const GVIEffect& params = *GVIEffect::effectsByIndex[m_id.id];
    const GVIEffect::GVIAnopheles& anoph = params.species[speciesIndex];
    double effect = (1.0 - anoph.preprandialKilling * getEffectSurvival(params));
    return anoph.byProtection( effect );
}

double HumanGVI::postprandialSurvivalFactor(size_t speciesIndex) const{
    const GVIEffect& params = *GVIEffect::effectsByIndex[m_id.id];
    const GVIEffect::GVIAnopheles& anoph = params.species[speciesIndex];
    double effect = (1.0 - anoph.postprandialKilling * getEffectSurvival(params));
    return anoph.byProtection( effect );
}

void HumanGVI::checkpoint( ostream& stream ){
    deployTime & stream;
    decayHet & stream;
}
HumanGVI::HumanGVI( istream& stream, EffectId id ) : PerHostInterventionData( id )
{
    deployTime & stream;
    decayHet & stream;
}

} }