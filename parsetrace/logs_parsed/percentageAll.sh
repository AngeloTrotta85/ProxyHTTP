for file in `find $1 -name "$2"`
do
	./percentage.py $file
done

