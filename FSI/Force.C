#include "Force.H"

using namespace Foam;

preciceAdapter::FSI::Force::Force
(
    const Foam::fvMesh& mesh,
    const fileName& timeName
    /* TODO: We should add any required field names here.
    /  They would need to be vector fields.
    /  See CHT/Temperature.C for details.
    */
)
:
mesh_(mesh)
{
    dataType_ = vector;

    // TODO: Is this ok?
    Force_ = new volVectorField
    (
        IOobject
        (
            "Force",
            timeName,
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedVector
        (
            "fdim",
            dimensionSet(1,1,-2,0,0,0,0),
            Foam::vector::zero
        )
    );
}

Foam::tmp<Foam::volSymmTensorField> preciceAdapter::FSI::Force::devRhoReff() const
{
    // TODO: Only works for laminar flows at the moment.
    // See the OpenFOAM Forces function object, where an extended
    // version of this method is being used.

    // Get the kinematic viscosity from the transportProperties
    // TODO: Get it from the objects' registry directly?
    const dictionary& transportProperties =
        mesh_.lookupObject<IOdictionary>("transportProperties");

    dimensionedScalar nu(transportProperties.lookup("nu"));

    // Get the velocity
    const volVectorField& U = mesh_.lookupObject<volVectorField>("U");

    // Get the density
    // TODO Check
    // TODO This probably only works with compressible solvers
    const volScalarField& rho = mesh_.lookupObject<volScalarField>("rho");

    return -rho * nu * dev(twoSymm(fvc::grad(U)));

}

void preciceAdapter::FSI::Force::write(double * buffer)
{
    /* TODO: Implement
    * We need two nested for-loops for each patch,
    * the outer for the locations and the inner for the dimensions.
    * See the preCICE writeBlockVectorData() implementation.
    */
    // Compute forces. See the Forces function object.
    // Normal vectors on the boundary, multiplied with the face areas
    const surfaceVectorField::Boundary& Sfb =
        mesh_.Sf().boundaryField();

    // Stress tensor
    tmp<volSymmTensorField> tdevRhoReff = devRhoReff();

    // Stress tensor boundary field
    const volSymmTensorField::Boundary& devRhoReffb =
        tdevRhoReff().boundaryField();

    // Pressure
    const volScalarField& p =
        mesh_.lookupObject<volScalarField>("p");

    // Density
    // TODO Check
    // TODO This probably only works with compressible solvers
    const volScalarField& rho = mesh_.lookupObject<volScalarField>("rho");

    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < patchIDs_.size(); j++)
    {
        int patchID = patchIDs_.at(j);

        // Pressure forces
        Force_->boundaryFieldRef()[patchID] =
            Sfb[patchID] * p.boundaryField()[patchID] * rho;

        // Viscous forces
        Force_->boundaryFieldRef()[patchID] +=
            Sfb[patchID] & devRhoReffb[patchID];

        // Write the forces to the preCICE buffer
        // For every cell of the patch
        forAll(Force_->boundaryFieldRef()[patchID], i)
        {
            // Copy the force into the buffer
            // x-dimension
            buffer[bufferIndex++]
            =
            Force_->boundaryFieldRef()[patchID][i].x();

            // y-dimension
            buffer[bufferIndex++]
            =
            Force_->boundaryFieldRef()[patchID][i].y();

            // z-dimension
            buffer[bufferIndex++]
            =
            Force_->boundaryFieldRef()[patchID][i].z();
        }
    }
}

void preciceAdapter::FSI::Force::read(double * buffer)
{
    /* TODO: Implement
    * We need two nested for-loops for each patch,
    * the outer for the locations and the inner for the dimensions.
    * See the preCICE readBlockVectorData() implementation.
    */
    FatalErrorInFunction
        << "Reading forces is not supported."
        << exit(FatalError);
}

preciceAdapter::FSI::Force::~Force()
{
    // TODO: Is this enough?
    delete Force_;
}