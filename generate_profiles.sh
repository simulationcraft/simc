
#15H
cd profiles/Tier15H/
FILES=generate_*

for f in $FILES
do
	../../simc $f
done

cd ../..

#14H
cd profiles/Tier14H/
FILES=generate_*

for f in $FILES
do
	../../simc $f
done

cd ../..

#14N
cd profiles/Tier14N/
FILES=generate_*

for f in $FILES
do
	../../simc $f
done

cd ../..

#15N
cd profiles/Tier15N/
FILES=generate_*

for f in $FILES
do
	../../simc $f
done

cd ../..