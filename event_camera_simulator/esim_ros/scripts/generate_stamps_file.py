import argparse
from os import listdir
from os.path import join
import numpy as np
from scipy import interpolate
import csv


if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='Generate "images.csv" for ESIM DataProviderFromFolder'
    )

    parser.add_argument(
        '-i',
        '--input_folder',
        default=None,
        type=str,
        help='folder containing the images',
    )
    parser.add_argument(
        '-r',
        '--framerate',
        default=1000,
        type=float,
        help='video framerate, in Hz',
    )

    args = parser.parse_args()

    images = sorted(
        [
            f
            for f in listdir(args.input_folder)
            if f.endswith('.exr') or f.endswith('.png')
        ]
    )

    print(
        f"Will write timestamps in file: {join(args.input_folder, 'images.csv')} with framerate: {args.framerate} Hz"
    )
    stamp_nanoseconds = 1
    dt_nanoseconds = int((1.0 / args.framerate) * 1e9)
    with open(join(args.input_folder, 'images.csv'), 'w') as f:
        for image_path in images:
            f.write('{},{}\n'.format(stamp_nanoseconds, image_path))
            stamp_nanoseconds += dt_nanoseconds

    print('Done!')
    print(
        f"Will write camera poses in file: {join(args.input_folder, 'images.csv')}"
    )
    fields = ['frame number', 'x', 'y', 'z', 'roll', 'pitch', 'yaw']
    with open(join(args.input_folder, 'poses.csv'), 'w') as poses:
        writer = csv.DictWriter(poses, fieldnames=fields)
        writer.writeheader()

        with open(join(args.input_folder, 'keyframes.csv'), 'r') as csv_file:
            reader = csv.DictReader(csv_file)
            for n, row in enumerate(reader):
                for field in row:
                    row[field] = float(row[field])
                if n == 0:
                    prev = row
                    continue
                start, end = prev['frame number'], row['frame number']
                time = np.arange(start, end)

                data = []
                for t in time:
                    data.append({f: 0 for f in fields})
                    data[-1]['frame number'] = int(t)

                for var in fields[1:]:
                    f = interpolate.CubicHermiteSpline(
                        [start, end], [prev[var], row[var]], [0, 0]
                    )

                    # verify(f, var, start, end)
                    for i, value in enumerate(f(time)):
                        data[i][var] = value
                    prev = row
                writer.writerows(data)
        print('Done!')
